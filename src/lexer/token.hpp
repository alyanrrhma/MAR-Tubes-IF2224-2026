#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

class TokenType {
private:
    static int next_id;
    int id;
    std::string name;

public:
    TokenType(std::string type_name);
    int get_type() const;
    const std::string& get_name() const;
};

class Token {
private:
    TokenType type;
    std::string value;

public:
    Token(TokenType type, std::string value = "");
    std::string to_string() const;
    const std::string& get_value() const;
    int get_type() const;
    const std::string& get_type_name() const;
};

#endif
