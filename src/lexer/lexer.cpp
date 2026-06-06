#include "lexer.hpp"

#include <cctype>
#include <iostream>
#include <optional>

Lexer::Lexer(std::istream& source, std::shared_ptr<DFA> automaton, std::ostream* output)
    : src(source),
      dfa(automaton),
      out(output),
      line_counter(1),
      col_counter(0),
      reached_eof(false)
{}

bool Lexer::eof() const {
    return reached_eof;
}

bool Lexer::read_char(char& c) {
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

static bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool isTokenBoundary(char c) {
    switch (c) {
        case ' ': case '\t': case '\r': case '\n':
        case '+': case '-': case '*': case '/':
        case '<': case '>': case '=': case ':':
        case '.': case ',': case ';':
        case '(': case ')': case '[': case ']':
        case '{': case '}': case '\'':
            return true;
        default:
            return false;
    }
}

static bool isValueLikeToken(const std::string& name) {
    return name == "IDENT" || name == "INTCON" ||
           name == "REALCON" || name == "RANGE";
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

void Lexer::write_range_tokens(const std::string& lexeme) {
    size_t dotdot = lexeme.find("..");
    std::string first  = lexeme.substr(0, dotdot);
    std::string second = lexeme.substr(dotdot + 2);
    TokenType intcon = dfa->getTokenTypeFromTypeName("INTCON");
    TokenType dot    = dfa->getTokenTypeFromTypeName("PERIOD");
    write_token(Token(intcon, first));
    write_token(Token(dot, "."));
    write_token(Token(dot, "."));
    write_token(Token(intcon, second));
}

void Lexer::write_token(const Token& t) {
    result.push_back(t);
    if (out != nullptr) {
        *out << t.to_string() << "\n";
    }
}

void Lexer::process_next_token() {
    if (reached_eof) {
        return;
    }

    TokenType unknownType = dfa->getTokenTypeFromTypeName("UNKNOWN");
    TokenType commentType = dfa->getTokenTypeFromTypeName("COMMENT");
    TokenType identType = dfa->getTokenTypeFromTypeName("IDENT");

    auto make_token = [&](TokenType tt, const std::string& lex) -> Token {
        const std::string& name = tt.get_name();
        if (name == "CHARCON" || name == "STRING") {
            return Token(tt, decodeQuotedValue(lex));
        }
        return Token(tt, lex);
    };

    auto emit_unknown = [&](const std::string& lex) {
        if (!lex.empty()) {
            write_token(Token(unknownType, lex));
        }
    };

    auto report_lexical_error = [&](const std::string& message,
                                    int line,
                                    int col,
                                    const std::string& lex) {
        std::cerr << "[" << line << ":" << col << "] lexical error: "
                  << message;
        if (!lex.empty()) {
            std::cerr << " (near '" << lex << "')";
        }
        std::cerr << "\n";
    };

    auto report_unclosed_quote = [&](int line, int col, const std::string& lex) {
        report_lexical_error("string/char literal belum ditutup", line, col, lex);
    };

    auto emit_accepted = [&](TokenType tt, const std::string& lex) {
        if (lex.empty()) {
            return;
        }
        if (tt.get_name() == "RANGE") {
            write_range_tokens(lex);
            return;
        }
        write_token(make_token(tt, lex));
    };

    std::optional<char> pending;

    auto scan_comment = [&](std::string lexeme, char prev, int start_line, int start_col) {
        char c;
        while (read_char(c)) {
            update_position(c);
            lexeme += c;
            if (c == '}' || (prev == '*' && c == ')')) {
                write_token(Token(commentType, lexeme));
                return;
            }
            prev = c;
        }

        report_lexical_error("unterminated comment", start_line, start_col, lexeme);
        emit_unknown(lexeme);
    };

    auto scan_unknown_tail = [&](std::string lexeme) {
        char c;
        while (read_char(c)) {
            update_position(c);
            if (isWhitespace(c)) {
                emit_unknown(lexeme);
                return;
            }
            if (isTokenBoundary(c)) {
                emit_unknown(lexeme);
                pending = c;
                return;
            }
            lexeme += c;
        }
        emit_unknown(lexeme);
    };

    auto scan_identifier_tail = [&](std::string lexeme) {
        char c;
        while (read_char(c)) {
            update_position(c);
            if (std::isalnum(static_cast<unsigned char>(c))) {
                lexeme += c;
                continue;
            }
            pending = c;
            break;
        }
        write_token(Token(identType, lexeme));
    };

    dfa->resetState();
    std::string lexeme;
    int token_start_line = line_counter;
    int token_start_col = col_counter;
    char c;

    while (pending.has_value() || read_char(c)) {
        if (pending.has_value()) {
            c = pending.value();
            pending = std::nullopt;
        } else {
            update_position(c);
        }

        bool consume_current = true;
        while (consume_current) {
            consume_current = false;

            if (lexeme.empty() && isWhitespace(c)) {
                dfa->resetState();
                break;
            }

            if (lexeme.empty()) {
                token_start_line = line_counter;
                token_start_col = col_counter;
            }

            if (lexeme.empty() && c == '{') {
                dfa->resetState();
                scan_comment("{", '{', token_start_line, token_start_col);
                break;
            }

            const State prev_state = dfa->getState();
            dfa->next(static_cast<unsigned char>(c));
            const State next_state = dfa->getState();

            if (!next_state.isNullState()) {
                TokenType next_token = dfa->getTokenForState(next_state.getStateIdx());
                if (next_state.isFinalState() && next_token.get_name() == "UNKNOWN" &&
                    !lexeme.empty() &&
                    std::isalpha(static_cast<unsigned char>(lexeme.front())) &&
                    std::isalnum(static_cast<unsigned char>(c))) {
                    lexeme += c;
                    dfa->resetState();
                    scan_identifier_tail(lexeme);
                    lexeme.clear();
                    break;
                }

                lexeme += c;

                if (lexeme == "(*") {
                    dfa->resetState();
                    scan_comment(lexeme, '*', token_start_line, token_start_col);
                    lexeme.clear();
                }
                break;
            }

            TokenType prev_token = dfa->getTokenForState(prev_state.getStateIdx());
            if (prev_state.isFinalState() && prev_token.get_name() != "UNKNOWN") {
                if (prev_token.get_name() == "IDENT" &&
                    std::isalnum(static_cast<unsigned char>(c))) {
                    lexeme += c;
                    dfa->resetState();
                    scan_identifier_tail(lexeme);
                    lexeme.clear();
                    break;
                }

                if (isValueLikeToken(prev_token.get_name()) && !isTokenBoundary(c)) {
                    emit_accepted(prev_token, lexeme);
                    lexeme.clear();
                    dfa->resetState();
                    consume_current = true;
                    continue;
                }

                emit_accepted(prev_token, lexeme);
                lexeme.clear();
                dfa->resetState();

                if (!isWhitespace(c)) {
                    consume_current = true;
                    continue;
                }
                break;
            }

            if (lexeme.empty()) {
                if (!isWhitespace(c)) {
                    emit_unknown(std::string(1, c));
                }
                dfa->resetState();
                break;
            }

            if (isWhitespace(c)) {
                if (!lexeme.empty() && lexeme.front() == '\'') {
                    report_unclosed_quote(token_start_line, token_start_col, lexeme);
                }
                emit_unknown(lexeme);
                lexeme.clear();
                dfa->resetState();
                break;
            }

            if (isTokenBoundary(c)) {
                emit_unknown(lexeme);
                lexeme.clear();
                dfa->resetState();
                consume_current = true;
                continue;
            }

            if (!lexeme.empty() &&
                std::isalpha(static_cast<unsigned char>(lexeme.front())) &&
                std::isalnum(static_cast<unsigned char>(c))) {
                lexeme += c;
                dfa->resetState();
                scan_identifier_tail(lexeme);
                lexeme.clear();
                break;
            }

            lexeme += c;
            dfa->resetState();
            scan_unknown_tail(lexeme);
            lexeme.clear();
            break;
        }
    }

    const State cur = dfa->getState();
    if (!lexeme.empty()) {
        if (cur.isFinalState()) {
            emit_accepted(dfa->getCurrToken(), lexeme);
        } else {
            if (lexeme.front() == '\'') {
                report_unclosed_quote(token_start_line, token_start_col, lexeme);
            }
            emit_unknown(lexeme);
        }
    }

    reached_eof = true;
}

const std::vector<Token>& Lexer::getResult() const {
    return result;
}
