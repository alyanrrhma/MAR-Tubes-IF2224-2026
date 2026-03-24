#include "token.hpp"
#include <set>

int TokenType::next_id = 0;

TokenType::TokenType(std::string type_name)
    : id(TokenType::next_id++), name(type_name) {}

int TokenType::get_type() { return id; }

const std::string& TokenType::get_name() { return name; }

bool token_has_value(const std::string& type_name) {
    static const std::set<std::string> valued_tokens = {
        "ident",
        "intcon",
        "realcon",
        "charcon",
        "string"
    };
    return valued_tokens.find(type_name) != valued_tokens.end();
}

Token::Token(TokenType type, std::string value)
    : type(type), value(value) {}

std::string Token::to_string() {
    std::string name = type.get_name();
    if (token_has_value(name) && !value.empty()) {
        return name + " (" + value + ")";
    }
    return name;
}

const std::string& Token::get_value() { return value; }

int Token::get_type() { return type.get_type(); }
