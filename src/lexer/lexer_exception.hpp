#ifndef LEXER_EXCEPTION_HPP
#define LEXER_EXCEPTION_HPP

#include <stdexcept>
#include <string>
#include <sstream>


class LexerException : public std::runtime_error {
public:
    LexerException(const std::string& message,
                   int line,
                   int col,
                   const std::string& lexeme = "");

    int get_line() const;
    int get_col() const;
    const std::string& get_lexeme() const;
    std::string full_message() const;

private:
    int err_line;
    int err_col;
    std::string err_lexeme;
};

inline LexerException::LexerException(const std::string& message,
                                      int line,
                                      int col,
                                      const std::string& lexeme)
    : std::runtime_error(message),
      err_line(line),
      err_col(col),
      err_lexeme(lexeme)
{}

inline int LexerException::get_line() const { return err_line; }
inline int LexerException::get_col()  const { return err_col;  }

inline const std::string& LexerException::get_lexeme() const { return err_lexeme; }

inline std::string LexerException::full_message() const {
    std::ostringstream oss;
    oss << "[" << err_line << ":" << err_col << "] ERROR: " << what();
    if (!err_lexeme.empty()) {
        oss << " (near '" << err_lexeme << "')";
    }
    return oss.str();
}

#endif 