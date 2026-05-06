#ifndef PARSER_HPP
#define PARSER_HPP

#include "../lexer/token.hpp"
#include "parse_tree.hpp"

#include <stdexcept>
#include <string>
#include <vector>

class ParseException : public std::runtime_error {
public:
    int line;
    int column;

    ParseException(const std::string& message, int line = -1, int column = -1)
        : std::runtime_error(message), line(line), column(column) {}

    std::string full_message() const {
        if (line >= 0 && column >= 0) {
            return "[ParseError " + std::to_string(line) + ":" +
                   std::to_string(column) + "] " + what();
        }
        return std::string("[ParseError] ") + what();
    }
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    parse_tree::NodePtr parse();

private:
    const std::vector<Token>& tokens;
    size_t pos;   
    bool            atEnd()                            const;
    const Token&    current()                          const;
    const Token&    lookAhead(size_t offset = 1)       const;
    bool            check(const std::string& typeName) const;
    bool            checkAny(std::initializer_list<std::string> types) const;

    Token consume();
    Token expect(const std::string& typeName);
    parse_tree::NodePtr termNode();
    parse_tree::NodePtr expectNode(const std::string& typeName);

    bool isConstantStart()  const;
    bool isStatementStart() const;
    bool isTypeStart()      const;
    bool isDeclarationStart() const;
    bool isRelationalOp()   const;
    bool isAdditiveOp()     const;
    bool isMultiplicativeOp() const;

    parse_tree::NodePtr parseProgram();
    parse_tree::NodePtr parseProgramHeader();
    parse_tree::NodePtr parseIdentifierList();

    parse_tree::NodePtr parseBlock();
    parse_tree::NodePtr parseDeclarationPart();

    parse_tree::NodePtr parseConstDeclaration();
    parse_tree::NodePtr parseConstDefinition();

    parse_tree::NodePtr parseTypeDeclaration();
    parse_tree::NodePtr parseTypeDefinition();

    parse_tree::NodePtr parseVarDeclaration();
    parse_tree::NodePtr parseVariableDeclaration();

    parse_tree::NodePtr parseType();
    parse_tree::NodePtr parseArrayType();
    parse_tree::NodePtr parseRange();
    parse_tree::NodePtr parseEnumerated();
    parse_tree::NodePtr parseRecordType();
    parse_tree::NodePtr parseFieldList();

    parse_tree::NodePtr parseSubprogramDeclarationPart();
    parse_tree::NodePtr parseSubprogramDeclaration();
    parse_tree::NodePtr parseProcedureDeclaration();
    parse_tree::NodePtr parseFunctionDeclaration();
    parse_tree::NodePtr parseProcedureHeading();
    parse_tree::NodePtr parseFunctionHeading();
    parse_tree::NodePtr parseFormalParameterList();
    parse_tree::NodePtr parseParameterGroup();

    parse_tree::NodePtr parseCompoundStatement();
    parse_tree::NodePtr parseStatementList();
    parse_tree::NodePtr parseStatement();
    parse_tree::NodePtr parseAssignmentStatement(parse_tree::NodePtr varNode);
    parse_tree::NodePtr parseProcedureFunctionCall();
    parse_tree::NodePtr parseIfStatement();
    parse_tree::NodePtr parseCaseStatement();
    parse_tree::NodePtr parseCaseBlock();
    parse_tree::NodePtr parseRepeatStatement();
    parse_tree::NodePtr parseWhileStatement();
    parse_tree::NodePtr parseForStatement();
    parse_tree::NodePtr parseParameterList();

    parse_tree::NodePtr parseExpression();
    parse_tree::NodePtr parseRelationalOperator();
    parse_tree::NodePtr parseSimpleExpression();
    parse_tree::NodePtr parseAdditiveOperator();
    parse_tree::NodePtr parseTerm();
    parse_tree::NodePtr parseMultiplicativeOperator();
    parse_tree::NodePtr parseFactor();

    parse_tree::NodePtr parseVariable();
    parse_tree::NodePtr parseSelector();
    parse_tree::NodePtr parseIndexList();
    parse_tree::NodePtr parseConstant();
};

#endif // PARSER_HPP