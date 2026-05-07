#include "lexer.hpp"

Lexer::Lexer(std::istream& source, std::shared_ptr<DFA> automaton, std::ostream* output)
    : src(source),
      dfa(automaton),
      out(output),
      line_counter(1),
      col_counter(0),
      reprocess_input(std::nullopt),
      reached_eof(false)
{}

bool Lexer::eof() const {
    if (reprocess_input.has_value()) return false;
    return reached_eof;
}

bool Lexer::read_char(char& c) {
    if (reprocess_input.has_value()) {
        c = reprocess_input.value();
        reprocess_input = std::nullopt;
        return true;
    }

    if (!src.get(c)) {
        reached_eof = true;
        return false;
    }
    return true;
}

void Lexer::update_position(char c) {
    if (c == '\n') {
        ++line_counter;
        col_counter = 0;
    } else {
        ++col_counter;
    }
}

static std::string decodeQuotedValue(const std::string& raw) {
    if (raw.size() < 2) return raw;
    std::string content = raw.substr(1, raw.size() - 2);
    std::string decoded;
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\'' && i + 1 < content.size() && content[i + 1] == '\'') {
            decoded += '\'';
            ++i;
        } else {
            decoded += content[i];
        }
    }
    return "'" + decoded + "'";
}

void Lexer::write_token(const Token& t) {
    result.push_back(t);
    if (out != nullptr) {
        *out << t.to_string() << "\n";
    }
}

void Lexer::process_next_token() {
    dfa->resetState();

    std::string lexeme;
    char c;

    auto write_unknown = [&]() {
        if (!lexeme.empty()) {
            Token tok(dfa->getCurrToken(), lexeme);
            write_token(tok);
        }
    };

    auto make_token = [&](TokenType tt, const std::string& lex) -> Token {
        const std::string& name = tt.get_name();
        if (name == "CHARCON" || name == "STRING") {
            return Token(tt, decodeQuotedValue(lex));
        }
        return Token(tt, lex);
    };

    auto munch_until_whitespace = [&]() {
        while (read_char(c)) {
            update_position(c);
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                break;
            }
            lexeme += c;
        }
        write_unknown();
    };

    while (true) {
        bool got_char = read_char(c);

        if (!got_char) {
            const State& cur = dfa->getState();

            if (cur.isNullState()) {
                if (!lexeme.empty()) {
                    write_unknown();
                    return;
                }
                return;
            }

            if (!cur.isFinalState()) {
                write_unknown();
                return;
            }

            TokenType tt = dfa->getCurrToken();
            if (tt.get_name() == "RANGE") {
                size_t dotdot = lexeme.find("..");
                std::string first = lexeme.substr(0, dotdot);
                std::string second = lexeme.substr(dotdot + 2);
                TokenType intcon = dfa->getTokenTypeFromTypeName("INTCON");
                TokenType dot = dfa->getTokenTypeFromTypeName("PERIOD");
                write_token(Token(intcon, first));
                write_token(Token(dot, "."));
                write_token(Token(dot, "."));
                write_token(Token(intcon, second));
                return;
            }
            write_token(make_token(tt, lexeme));
            return;
        }

        update_position(c);

        if (lexeme.empty() && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
            dfa->resetState();
            continue;
        }

        const State& prev_state = dfa->getState();
        dfa->next(static_cast<unsigned char>(c));
        const State& next_state = dfa->getState();

        if (next_state.isNullState()) {
            TokenType tt = dfa->getTokenForState(prev_state.getStateIdx());
            if (prev_state.isFinalState() && tt.get_name() != "UNKNOWN") {
                reprocess_input = c;
                if (c == '\n') {
                    --line_counter;
                    col_counter = 0;
                } else {
                    --col_counter;
                }

                if (tt.get_name() == "RANGE") {
                    size_t dotdot = lexeme.find("..");
                    std::string first = lexeme.substr(0, dotdot);
                    std::string second = lexeme.substr(dotdot + 2);
                    TokenType intcon = dfa->getTokenTypeFromTypeName("INTCON");
                    TokenType dot = dfa->getTokenTypeFromTypeName("PERIOD");
                    write_token(Token(intcon, first));
                    write_token(Token(dot, "."));
                    write_token(Token(dot, "."));
                    write_token(Token(intcon, second));
                    return;
                }
                write_token(make_token(tt, lexeme));
                return;
            }

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                write_unknown();
                return;
            }

            lexeme += c;
            munch_until_whitespace();
            return;
        }

        if ((c == ' ' || c == '\t' || c == '\r' || c == '\n') &&
            next_state.isFinalState() &&
            dfa->getTokenForState(next_state.getStateIdx()).get_name() == "UNKNOWN") {
            write_unknown();
            return;
        }

        lexeme += c;
    }
}

const std::vector<Token>& Lexer::getResult() const {
    return result;
}