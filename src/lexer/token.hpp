#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

// TokenType — jenis token bahasa Arion
// Didefinisikan dinamis via config file
class TokenType {
public:
    TokenType(std::string type_name);
    int get_type();
    const std::string& get_name();
private:
    static int next_id;
    int id;
    std::string name;
};

// Token — satuan hasil tokenisasi
// Produced by Lexer using DFA
class Token {
public:
    Token(TokenType type, std::string value);
    std::string to_string();       
    const std::string& get_value();
    int get_type();
private:
    std::string value;
    TokenType type;
};

// Helper — apakah token jenis ini perlu mencetak nilai?
// Token yang perlu nilai  : ident, intcon, realcon, charcon, string
// Token yang tidak perlu  : semua keyword, operator, tanda baca
bool token_has_value(const std::string& type_name);

#endif
