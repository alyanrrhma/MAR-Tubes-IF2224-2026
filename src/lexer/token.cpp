#include "token.hpp"
#include <algorithm>
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

static bool token_has_value(const std::string& type_name) {
    static const std::set<std::string> valued_tokens = {
        "IDENT", "INTCON", "REALCON", "CHARCON", "STRING", "UNKNOWN", "COMMENT"
    };
    return valued_tokens.find(type_name) != valued_tokens.end();
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

Token::Token(TokenType type, std::string value)
    : type(type), value(value) {}

std::string Token::to_string() const {
    const std::string& name = type.get_name();
    std::string display = to_lower(name);
    if (token_has_value(name) && !value.empty()) {
        return display + " (" + value + ")";
    }
    return display;
}

const std::string& Token::get_value() const { 
    return value; 
}

int Token::get_type() const { 
    return type.get_type(); 
}

const std::string& Token::get_type_name() const { 
    return type.get_name(); 
}