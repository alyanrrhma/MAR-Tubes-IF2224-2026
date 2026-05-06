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
    // Masih ada karakter pending → belum benar-benar EOF
    if (reprocess_input.has_value()) return false;
    return reached_eof;
}

bool Lexer::read_char(char& c) {
    // Jika ada karakter yang tertunda untuk diproses ulang, kembalikan itu dulu
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


void Lexer::write_token(const Token& t) {
    result.push_back(t);
    if (out != nullptr) {
        *out << t.to_string() << "\n";
    }
}

// Mendapatkan token berikutnya
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

        // Misalkan EOF di tengah
        if (!got_char) {
            // Jika DFA sedang di final state, token selesai karena EOF
            const State& cur = dfa->getState();

            if (cur.isNullState()) {
                // DFA di null state dan EOF → karakter jadi karakter tidak dikenali
                if (!lexeme.empty()) {
                    write_unknown();
                    return;
                }
                
                return;
            }

            if (!cur.isFinalState()) {
                // Input habis tapi token belum selesai 
                write_unknown();
                return;
            }

            // Sudah mencapai final state
            TokenType tt = dfa->getCurrToken();
            // if (tt.get_name() == "IDENT" && dfa->hasKeywordToken(lexeme)) {
            //     tt = dfa->getKeywordToken(lexeme);
            // }
            if (tt.get_name() == "RANGE") {
                size_t dotdot = lexeme.find("..");
                std::string first = lexeme.substr(0, dotdot);
                std::string second = lexeme.substr(dotdot + 2);
                TokenType intcon = dfa->getTokenTypeFromTypeName("INTCON");
                TokenType dot = dfa->getTokenTypeFromTypeName("PERIOD");
                Token firstInt = Token(intcon, first);
                Token firstDot = Token(dot, ".");
                Token secondDot = Token(dot, ".");
                Token secondInt = Token(intcon, second);
                write_token(firstInt);
                write_token(firstDot);
                write_token(secondDot);
                write_token(secondInt);
                return;
            }
            Token tok(tt, lexeme);
            write_token(tok);
            return;
        }

        update_position(c);

        if (lexeme.empty() && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
            // Whitespace di antara token: reset state dan perbarui token_start
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

                // if (tt.get_name() == "IDENT" && dfa->hasKeywordToken(lexeme)) {
                //     tt = dfa->getKeywordToken(lexeme);
                // }
                if (tt.get_name() == "RANGE") {
                    size_t dotdot = lexeme.find("..");
                    std::string first = lexeme.substr(0, dotdot);
                    std::string second = lexeme.substr(dotdot + 2);
                    TokenType intcon = dfa->getTokenTypeFromTypeName("INTCON");
                    TokenType dot = dfa->getTokenTypeFromTypeName("PERIOD");
                    Token firstInt = Token(intcon, first);
                    Token firstDot = Token(dot, ".");
                    Token secondDot = Token(dot, ".");
                    Token secondInt = Token(intcon, second);
                    write_token(firstInt);
                    write_token(firstDot);
                    write_token(secondDot);
                    write_token(secondInt);
                    return;
                }
                Token tok(tt, lexeme);
                write_token(tok);
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

const std::vector<Token>& Lexer::getResult() const{
    return result;
}
