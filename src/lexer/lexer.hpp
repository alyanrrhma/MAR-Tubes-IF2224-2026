#ifndef LEXER_HPP
#define LEXER_HPP

#include <istream>
#include <ostream>
#include <memory>
#include <optional>
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

private:
    std::istream& src;
    std::shared_ptr<DFA> dfa;
    std::ostream* out;

    int  line_counter; // Baris yang sedang/sudah dibaca   
    int  col_counter;  // Kolom yang sedang/sudah dibaca

    std::optional<char> reprocess_input; // Karakter yang harus diproses ulang pada pemanggilan berikutnya
    bool reached_eof;     // Penanda apakah sudah sampai EOF

    bool read_char(char& c);
    void update_position(char c);
    void write_token(const Token& t);

    std::vector<Token> result;
};

#endif 