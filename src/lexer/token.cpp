#include "token.hpp"
#include <set>

int TokenType::next_id = 0;

TokenType::TokenType(std::string type_name)
    : id(TokenType::next_id++), name(type_name) {}

int TokenType::get_type() const { 
    return id; 
}

const std::string& TokenType::get_name() const { 
    return name; 
}

bool token_has_value(const std::string& type_name) {
    static const std::set<std::string> valued_tokens = {
        "ident", "IDENT",
        "intcon", "INTCON",
        "realcon", "REALCON",
        "charcon", "CHARCON",
        "string", "STRING",
        "unknown", "UNKNOWN",
        "comment", "COMMENT"
    };
    return valued_tokens.find(type_name) != valued_tokens.end();
}

Token::Token(TokenType type, std::string value)
    : type(type), value(value) {}

std::string Token::to_string() const {
    std::string name = type.get_name();
    if (token_has_value(name) && !value.empty()) {
        return name + " (" + value + ")";
    }
    return name;
}

const std::string& Token::get_value() const { 
    return value; 
}

int Token::get_type() const { 
    return type.get_type(); 
}