#include "parser.hpp"
#include <sstream>
#include <stdexcept>

using namespace parse_tree;

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), pos(0) {}

bool Parser::atEnd() const {
    return pos >= tokens.size();
}

const Token& Parser::current() const {
    if (atEnd()) {
        static Token eof_token(TokenType("EOF"), "");
        return eof_token;
    }
    return tokens[pos];
}

const Token& Parser::lookAhead(size_t offset) const {
    size_t idx = pos + offset;
    if (idx >= tokens.size()) {
        static Token eof_token(TokenType("EOF"), "");
        return eof_token;
    }
    return tokens[idx];
}

bool Parser::check(const std::string& typeName) const {
    if (atEnd()) return typeName == "EOF";
    return current().get_type_name() == typeName;
}

bool Parser::checkAny(std::initializer_list<std::string> types) const {
    for (const auto& t : types) {
        if (check(t)) return true;
    }
    return false;
}

Token Parser::consume() {
    if (atEnd()) {
        throw ParseException("Token tidak terduga: sudah mencapai akhir input");
    }
    return tokens[pos++];
}

Token Parser::expect(const std::string& typeName) {
    if (!check(typeName)) {
        std::string got = atEnd() ? "EOF" : current().get_type_name();
        std::string val = atEnd() ? "" : current().get_value();
        std::ostringstream msg;
        msg << "Diharapkan '" << typeName << "', mendapat '" << got << "'";
        if (!val.empty()) msg << " (" << val << ")";
        throw ParseException(msg.str());
    }
    return consume();
}

NodePtr Parser::termNode() {
    Token tok = consume();
    return makeTerminal(tok);
}

NodePtr Parser::expectNode(const std::string& typeName) {
    Token tok = expect(typeName);
    return makeTerminal(tok);
}

bool Parser::isConstantStart() const {
    return checkAny({"PLUS", "MINUS", "IDENT", "INTCON", "REALCON", "CHARCON", "STRING"});
}

bool Parser::isStatementStart() const {
    return checkAny({
        "IDENT", "BEGINSY", "IFSY", "CASESY",
        "REPEATSY", "WHILESY", "FORSY"
    });
}

bool Parser::isTypeStart() const {
    return checkAny({"IDENT", "ARRAYSY", "RECORDSY", "LPARENT"});
}

bool Parser::isDeclarationStart() const {
    return checkAny({"CONSTSY", "TYPESY", "VARSY", "PROCEDURESY", "FUNCTIONSY"});
}

bool Parser::isRelationalOp() const {
    return checkAny({"EQL", "NEQ", "LSS", "LEQ", "GTR", "GEQ"});
}

bool Parser::isAdditiveOp() const {
    return checkAny({"PLUS", "MINUS", "ORSY"});
}

bool Parser::isMultiplicativeOp() const {
    return checkAny({"TIMES", "IDIV", "RDIV", "IMOD", "ANDSY"});
}

NodePtr Parser::parse() {
    return parseProgram();
}

NodePtr Parser::parseProgram() {
    NodePtr node = makeNonTerminal(NonTerminal::Program);
    node->addChild(parseProgramHeader());
    node->addChild(parseDeclarationPart());
    node->addChild(parseCompoundStatement());
    node->addChild(expectNode("PERIOD"));
    return node;
}

NodePtr Parser::parseProgramHeader() {
    NodePtr node = makeNonTerminal(NonTerminal::ProgramHeader);
    node->addChild(expectNode("PROGRAMSY"));
    node->addChild(expectNode("IDENT"));
    node->addChild(expectNode("SEMICOLON"));
    return node;
}

NodePtr Parser::parseIdentifierList() {
    NodePtr node = makeNonTerminal(NonTerminal::IdentifierList);
    node->addChild(expectNode("IDENT"));
    while (check("COMMA")) {
        node->addChild(termNode());
        node->addChild(expectNode("IDENT"));
    }
    return node;
}

NodePtr Parser::parseBlock() {
    NodePtr node = makeNonTerminal(NonTerminal::Block);
    node->addChild(parseDeclarationPart());
    node->addChild(parseCompoundStatement());
    return node;
}

NodePtr Parser::parseDeclarationPart() {
    NodePtr node = makeNonTerminal(NonTerminal::DeclarationPart);
    while (isDeclarationStart()) {
        if (check("CONSTSY")) {
            node->addChild(parseConstDeclaration());
        } else if (check("TYPESY")) {
            node->addChild(parseTypeDeclaration());
        } else if (check("VARSY")) {
            node->addChild(parseVarDeclaration());
        } else {
            node->addChild(parseSubprogramDeclarationPart());
        }
    }
    return node;
}

NodePtr Parser::parseConstDeclaration() {
    NodePtr node = makeNonTerminal(NonTerminal::ConstDeclaration);
    node->addChild(expectNode("CONSTSY"));
    node->addChild(parseConstDefinition());
    node->addChild(expectNode("SEMICOLON"));
    while (check("IDENT") && lookAhead(1).get_type_name() == "EQL") {
        node->addChild(parseConstDefinition());
        node->addChild(expectNode("SEMICOLON"));
    }
    return node;
}

NodePtr Parser::parseConstDefinition() {
    NodePtr node = makeNonTerminal(NonTerminal::ConstDefinition);
    node->addChild(expectNode("IDENT"));
    node->addChild(expectNode("EQL"));
    node->addChild(parseConstant());
    return node;
}

NodePtr Parser::parseTypeDeclaration() {
    NodePtr node = makeNonTerminal(NonTerminal::TypeDeclaration);
    node->addChild(expectNode("TYPESY"));
    node->addChild(parseTypeDefinition());
    node->addChild(expectNode("SEMICOLON"));
    while (check("IDENT") && lookAhead(1).get_type_name() == "EQL") {
        node->addChild(parseTypeDefinition());
        node->addChild(expectNode("SEMICOLON"));
    }
    return node;
}

NodePtr Parser::parseTypeDefinition() {
    NodePtr node = makeNonTerminal(NonTerminal::TypeDefinition);
    node->addChild(expectNode("IDENT"));
    node->addChild(expectNode("EQL"));
    node->addChild(parseType());
    return node;
}

NodePtr Parser::parseVarDeclaration() {
    NodePtr node = makeNonTerminal(NonTerminal::VarDeclaration);
    node->addChild(expectNode("VARSY"));
    node->addChild(parseIdentifierList());
    node->addChild(expectNode("COLON"));
    node->addChild(parseType());
    node->addChild(expectNode("SEMICOLON"));
    while (check("IDENT") && !checkAny({"CONSTSY","TYPESY","PROCEDURESY","FUNCTIONSY","BEGINSY"})) {
        node->addChild(parseIdentifierList());
        node->addChild(expectNode("COLON"));
        node->addChild(parseType());
        node->addChild(expectNode("SEMICOLON"));
    }
    return node;
}

NodePtr Parser::parseFieldPart() {
    NodePtr node = makeNonTerminal(NonTerminal::FieldPart);
    node->addChild(parseIdentifierList());
    node->addChild(expectNode("COLON"));
    node->addChild(parseType());
    return node;
}

NodePtr Parser::parseType() {
    NodePtr node = makeNonTerminal(NonTerminal::Type);
    if (check("ARRAYSY")) {
        node->addChild(parseArrayType());
    } else if (check("RECORDSY")) {
        node->addChild(parseRecordType());
    } else if (check("LPARENT")) {
        node->addChild(parseEnumerated());
    } else if (check("IDENT")) {
        if (lookAhead(1).get_type_name() == "PERIOD" && lookAhead(2).get_type_name() == "PERIOD") {
            node->addChild(parseRange());
        } else {
            node->addChild(termNode());
        }
    } else if (checkAny({"INTCON", "REALCON", "CHARCON", "STRING", "PLUS", "MINUS"})) {
        node->addChild(parseRange());
    } else {
        throw ParseException(
            "Diharapkan tipe data (ident, array, record, range, atau enumerasi), mendapat '" +
            current().get_type_name() + "'");
    }
    return node;
}

NodePtr Parser::parseArrayType() {
    NodePtr node = makeNonTerminal(NonTerminal::ArrayType);
    node->addChild(expectNode("ARRAYSY"));
    node->addChild(expectNode("LBRACK"));
    node->addChild(parseRange());
    while (check("COMMA")) {
        node->addChild(termNode());
        node->addChild(parseRange());
    }
    node->addChild(expectNode("RBRACK"));
    node->addChild(expectNode("OFSY"));
    node->addChild(parseType());
    return node;
}

NodePtr Parser::parseRange() {
    NodePtr node = makeNonTerminal(NonTerminal::Range);
    node->addChild(parseConstant());
    node->addChild(expectNode("PERIOD"));
    node->addChild(expectNode("PERIOD"));
    node->addChild(parseConstant());
    return node;
}

NodePtr Parser::parseEnumerated() {
    NodePtr node = makeNonTerminal(NonTerminal::Enumerated);
    node->addChild(expectNode("LPARENT"));
    node->addChild(parseIdentifierList());
    node->addChild(expectNode("RPARENT"));
    return node;
}

NodePtr Parser::parseRecordType() {
    NodePtr node = makeNonTerminal(NonTerminal::RecordType);
    node->addChild(expectNode("RECORDSY"));
    node->addChild(parseFieldList());
    node->addChild(expectNode("ENDSY"));
    return node;
}

NodePtr Parser::parseFieldList() {
    NodePtr node = makeNonTerminal(NonTerminal::FieldList);
    node->addChild(parseFieldPart());
    while (check("SEMICOLON") && lookAhead(1).get_type_name() == "IDENT" &&
           lookAhead(2).get_type_name() != "CONSTSY" &&
           lookAhead(2).get_type_name() != "TYPESY") {
        if (check("SEMICOLON") && lookAhead(1).get_type_name() == "ENDSY") break;
        node->addChild(termNode());
        node->addChild(parseFieldPart());
    }
    if (check("SEMICOLON")) {
        node->addChild(termNode());
    }
    return node;
}

NodePtr Parser::parseSubprogramDeclarationPart() {
    NodePtr node = makeNonTerminal(NonTerminal::SubprogramDeclarationPart);
    node->addChild(parseSubprogramDeclaration());
    node->addChild(expectNode("SEMICOLON"));
    while (checkAny({"PROCEDURESY", "FUNCTIONSY"})) {
        node->addChild(parseSubprogramDeclaration());
        node->addChild(expectNode("SEMICOLON"));
    }
    return node;
}

NodePtr Parser::parseSubprogramDeclaration() {
    NodePtr node = makeNonTerminal(NonTerminal::SubprogramDeclaration);
    if (check("PROCEDURESY")) {
        node->addChild(parseProcedureDeclaration());
    } else if (check("FUNCTIONSY")) {
        node->addChild(parseFunctionDeclaration());
    } else {
        throw ParseException(
            "Diharapkan 'procedure' atau 'function', mendapat '" +
            current().get_type_name() + "'");
    }
    return node;
}

NodePtr Parser::parseProcedureDeclaration() {
    NodePtr node = makeNonTerminal(NonTerminal::ProcedureDeclaration);
    node->addChild(parseProcedureHeading());
    node->addChild(expectNode("SEMICOLON"));
    node->addChild(parseBlock());
    return node;
}

NodePtr Parser::parseFunctionDeclaration() {
    NodePtr node = makeNonTerminal(NonTerminal::FunctionDeclaration);
    node->addChild(parseFunctionHeading());
    node->addChild(expectNode("SEMICOLON"));
    node->addChild(parseBlock());
    return node;
}

NodePtr Parser::parseProcedureHeading() {
    NodePtr node = makeNonTerminal(NonTerminal::ProcedureHeading);
    node->addChild(expectNode("PROCEDURESY"));
    node->addChild(expectNode("IDENT"));
    if (check("LPARENT")) {
        node->addChild(parseFormalParameterList());
    }
    return node;
}

NodePtr Parser::parseFunctionHeading() {
    NodePtr node = makeNonTerminal(NonTerminal::FunctionHeading);
    node->addChild(expectNode("FUNCTIONSY"));
    node->addChild(expectNode("IDENT"));
    if (check("LPARENT")) {
        node->addChild(parseFormalParameterList());
    }
    node->addChild(expectNode("COLON"));
    node->addChild(expectNode("IDENT"));
    return node;
}

NodePtr Parser::parseFormalParameterList() {
    NodePtr node = makeNonTerminal(NonTerminal::FormalParameterList);
    node->addChild(expectNode("LPARENT"));
    node->addChild(parseParameterGroup());
    while (check("SEMICOLON")) {
        node->addChild(termNode());
        node->addChild(parseParameterGroup());
    }
    node->addChild(expectNode("RPARENT"));
    return node;
}

NodePtr Parser::parseParameterGroup() {
    NodePtr node = makeNonTerminal(NonTerminal::ParameterGroup);
    node->addChild(parseIdentifierList());
    node->addChild(expectNode("COLON"));
    if (check("ARRAYSY")) {
        node->addChild(parseArrayType());
    } else {
        node->addChild(expectNode("IDENT"));
    }
    return node;
}

NodePtr Parser::parseCompoundStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::CompoundStatement);
    node->addChild(expectNode("BEGINSY"));
    node->addChild(parseStatementList());
    node->addChild(expectNode("ENDSY"));
    return node;
}

NodePtr Parser::parseStatementList() {
    NodePtr node = makeNonTerminal(NonTerminal::StatementList);
    node->addChild(parseStatement());
    while (check("SEMICOLON")) {
        node->addChild(termNode());
        node->addChild(parseStatement());
    }
    return node;
}

NodePtr Parser::parseStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::Statement);
    if (check("IDENT")) {
        const std::string& next = lookAhead(1).get_type_name();
        if (next == "BECOMES") {
            NodePtr varNode = parseVariable();
            node->addChild(parseAssignmentStatement(std::move(varNode)));
        } else if (next == "LBRACK" || next == "PERIOD") {
            NodePtr varNode = parseVariable();
            if (check("BECOMES")) {
                node->addChild(parseAssignmentStatement(std::move(varNode)));
            } else {
                NodePtr callNode = makeNonTerminal(NonTerminal::ProcedureFunctionCall);
                callNode->addChild(std::move(varNode));
                node->addChild(std::move(callNode));
            }
        } else {
            node->addChild(parseProcedureFunctionCall());
        }
    } else if (check("BEGINSY")) {
        node->addChild(parseCompoundStatement());
    } else if (check("IFSY")) {
        node->addChild(parseIfStatement());
    } else if (check("CASESY")) {
        node->addChild(parseCaseStatement());
    } else if (check("REPEATSY")) {
        node->addChild(parseRepeatStatement());
    } else if (check("WHILESY")) {
        node->addChild(parseWhileStatement());
    } else if (check("FORSY")) {
        node->addChild(parseForStatement());
    } else {
        node->addChild(makeNonTerminal(NonTerminal::Empty));
    }
    return node;
}

NodePtr Parser::parseAssignmentStatement(NodePtr varNode) {
    NodePtr node = makeNonTerminal(NonTerminal::AssignmentStatement);
    node->addChild(std::move(varNode));
    node->addChild(expectNode("BECOMES"));
    node->addChild(parseExpression());
    return node;
}

NodePtr Parser::parseProcedureFunctionCall() {
    NodePtr node = makeNonTerminal(NonTerminal::ProcedureFunctionCall);
    node->addChild(expectNode("IDENT"));
    if (check("LPARENT")) {
        node->addChild(termNode());
        if (!check("RPARENT")) {
            node->addChild(parseParameterList());
        }
        node->addChild(expectNode("RPARENT"));
    }
    return node;
}

NodePtr Parser::parseIfStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::IfStatement);
    node->addChild(expectNode("IFSY"));
    node->addChild(parseExpression());
    node->addChild(expectNode("THENSY"));
    node->addChild(parseStatement());
    if (check("ELSESY")) {
        node->addChild(termNode());
        node->addChild(parseStatement());
    }
    return node;
}

NodePtr Parser::parseCaseStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::CaseStatement);
    node->addChild(expectNode("CASESY"));
    node->addChild(parseExpression());
    node->addChild(expectNode("OFSY"));
    node->addChild(parseCaseBlock());
    while (check("SEMICOLON") && !checkAny({"ENDSY","EOF"})) {
        if (lookAhead(1).get_type_name() == "ENDSY") break;
        node->addChild(termNode());
        node->addChild(parseCaseBlock());
    }
    if (check("SEMICOLON")) {
        node->addChild(termNode());
    }
    node->addChild(expectNode("ENDSY"));
    return node;
}

NodePtr Parser::parseCaseBlock() {
    NodePtr node = makeNonTerminal(NonTerminal::CaseBlock);
    node->addChild(parseConstant());
    while (check("COMMA")) {
        node->addChild(termNode());
        node->addChild(parseConstant());
    }
    node->addChild(expectNode("COLON"));
    node->addChild(parseStatement());
    return node;
}

NodePtr Parser::parseRepeatStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::RepeatStatement);
    node->addChild(expectNode("REPEATSY"));
    node->addChild(parseStatementList());
    node->addChild(expectNode("UNTILSY"));
    node->addChild(parseExpression());
    return node;
}

NodePtr Parser::parseWhileStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::WhileStatement);
    node->addChild(expectNode("WHILESY"));
    node->addChild(parseExpression());
    node->addChild(expectNode("DOSY"));
    node->addChild(parseStatement());
    return node;
}

NodePtr Parser::parseForStatement() {
    NodePtr node = makeNonTerminal(NonTerminal::ForStatement);
    node->addChild(expectNode("FORSY"));
    node->addChild(expectNode("IDENT"));
    node->addChild(expectNode("BECOMES"));
    node->addChild(parseExpression());
    if (check("TOSY")) {
        node->addChild(termNode());
    } else if (check("DOWNTOSY")) {
        node->addChild(termNode());
    } else {
        throw ParseException(
            "Diharapkan 'to' atau 'downto' dalam for-statement, mendapat '" +
            current().get_type_name() + "'");
    }
    node->addChild(parseExpression());
    node->addChild(expectNode("DOSY"));
    node->addChild(parseStatement());
    return node;
}

NodePtr Parser::parseParameterList() {
    NodePtr node = makeNonTerminal(NonTerminal::ParameterList);
    node->addChild(parseExpression());
    while (check("COMMA")) {
        node->addChild(termNode());
        node->addChild(parseExpression());
    }
    return node;
}

NodePtr Parser::parseExpression() {
    NodePtr node = makeNonTerminal(NonTerminal::Expression);
    node->addChild(parseSimpleExpression());
    if (isRelationalOp()) {
        node->addChild(parseRelationalOperator());
        node->addChild(parseSimpleExpression());
    }
    return node;
}

NodePtr Parser::parseRelationalOperator() {
    NodePtr node = makeNonTerminal(NonTerminal::RelationalOperator);
    if (isRelationalOp()) {
        node->addChild(termNode());
    } else {
        throw ParseException(
            "Diharapkan operator relasional, mendapat '" +
            current().get_type_name() + "'");
    }
    return node;
}

NodePtr Parser::parseSimpleExpression() {
    NodePtr node = makeNonTerminal(NonTerminal::SimpleExpression);
    if (checkAny({"PLUS", "MINUS"})) {
        node->addChild(termNode());
    }
    node->addChild(parseTerm());
    while (isAdditiveOp()) {
        node->addChild(parseAdditiveOperator());
        node->addChild(parseTerm());
    }
    return node;
}

NodePtr Parser::parseAdditiveOperator() {
    NodePtr node = makeNonTerminal(NonTerminal::AdditiveOperator);
    node->addChild(termNode());
    return node;
}

NodePtr Parser::parseTerm() {
    NodePtr node = makeNonTerminal(NonTerminal::Term);
    node->addChild(parseFactor());
    while (isMultiplicativeOp()) {
        node->addChild(parseMultiplicativeOperator());
        node->addChild(parseFactor());
    }
    return node;
}

NodePtr Parser::parseMultiplicativeOperator() {
    NodePtr node = makeNonTerminal(NonTerminal::MultiplicativeOperator);
    node->addChild(termNode());
    return node;
}

NodePtr Parser::parseFactor() {
    NodePtr node = makeNonTerminal(NonTerminal::Factor);
    if (check("LPARENT")) {
        node->addChild(termNode());
        node->addChild(parseExpression());
        node->addChild(expectNode("RPARENT"));
    } else if (check("NOTSY")) {
        node->addChild(termNode());
        node->addChild(parseFactor());
    } else if (check("IDENT")) {
        if (lookAhead(1).get_type_name() == "LPARENT") {
            NodePtr callNode = makeNonTerminal(NonTerminal::ProcedureFunctionCall);
            callNode->addChild(expectNode("IDENT"));
            callNode->addChild(termNode());
            if (!check("RPARENT")) {
                callNode->addChild(parseParameterList());
            }
            callNode->addChild(expectNode("RPARENT"));
            node->addChild(std::move(callNode));
        } else {
            node->addChild(parseVariable());
        }
    } else if (checkAny({"INTCON", "REALCON", "CHARCON", "STRING"})) {
        node->addChild(termNode());
    } else {
        throw ParseException(
            "Diharapkan factor (ekspresi, konstanta, variable, atau NOT), mendapat '" +
            current().get_type_name() + "'");
    }
    return node;
}

NodePtr Parser::parseVariable() {
    NodePtr node = makeNonTerminal(NonTerminal::Variable);
    node->addChild(expectNode("IDENT"));
    while (checkAny({"LBRACK", "PERIOD"})) {
        node->addChild(parseSelector());
    }
    return node;
}

NodePtr Parser::parseSelector() {
    NodePtr node = makeNonTerminal(NonTerminal::Selector);
    if (check("LBRACK")) {
        node->addChild(termNode());
        node->addChild(parseIndexList());
        node->addChild(expectNode("RBRACK"));
    } else if (check("PERIOD")) {
        node->addChild(termNode());
        node->addChild(expectNode("IDENT"));
    } else {
        throw ParseException(
            "Diharapkan '[' atau '.' untuk selector, mendapat '" +
            current().get_type_name() + "'");
    }
    return node;
}

NodePtr Parser::parseIndexList() {
    NodePtr node = makeNonTerminal(NonTerminal::IndexList);
    node->addChild(parseExpression());
    while (check("COMMA")) {
        node->addChild(termNode());
        node->addChild(parseExpression());
    }
    return node;
}

NodePtr Parser::parseConstant() {
    NodePtr node = makeNonTerminal(NonTerminal::Constant);
    if (checkAny({"PLUS", "MINUS"})) {
        node->addChild(termNode());
    }
    if (checkAny({"IDENT", "INTCON", "REALCON", "CHARCON", "STRING"})) {
        node->addChild(termNode());
    } else {
        throw ParseException(
            "Diharapkan konstanta (ident/intcon/realcon/charcon/string), mendapat '" +
            current().get_type_name() + "'");
    }
    return node;
}