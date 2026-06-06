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

    bool hasErrors() const;
    const std::vector<std::string>& getErrors() const;
    void enableTrace(std::ostream* traceOutput);

private:
    std::istream& src;
    std::shared_ptr<DFA> dfa;
    std::ostream* out;
    std::ostream* traceOut;

    int line_counter;
    int col_counter;
    bool reached_eof;

    bool read_char(char& c);
    void update_position(char c);
    void write_token(const Token& t);
    void write_range_tokens(const std::string& lexeme);
    void add_error(const std::string& message, int line, int col, const std::string& lexeme);
    void trace_transition(char c, const State& from, const State& to, const std::string& currentLexeme) const;

    std::vector<Token> result;
    std::vector<std::string> errors;
};

#endif
