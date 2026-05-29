#ifndef LEXER_HPP
#define LEXER_HPP

#include <istream>
#include <ostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "dfa.hpp"
#include "token.hpp"
#include "lexer_exception.hpp"

class Lexer {
public:
    Lexer(std::istream& source, std::shared_ptr<DFA> automaton, std::ostream* output = nullptr);
    bool eof() const;
    void process_next_token();
    const std::vector<Token>& getResult() const;

private:
    std::istream& src;
    std::shared_ptr<DFA> dfa;
    std::ostream* out;

    int line_counter;
    int col_counter;
    bool reached_eof;

    bool read_char(char& c);
    void update_position(char c);
    void write_token(const Token& t);
    void write_range_tokens(const std::string& lexeme);

    std::vector<Token> result;
};

#endif
