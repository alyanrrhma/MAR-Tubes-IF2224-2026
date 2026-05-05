#include "parse_tree.hpp"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace parse_tree {

Node::Node(NodeKind kind, std::string label, std::string value, int line, int column)
    : kind(kind), label(std::move(label)), value(std::move(value)), line(line), column(column) {}

Node* Node::addChild(Node* child) {
    if (child == nullptr) {
        return nullptr;
    }
    children.emplace_back(child);
    return children.back().get();
}

Node* Node::addChild(std::unique_ptr<Node> child) {
    if (!child) {
        return nullptr;
    }
    children.push_back(std::move(child));
    return children.back().get();
}

NodeKind Node::getKind() const {
    return kind;
}

bool Node::isTerminal() const {
    return kind == NodeKind::Terminal;
}

bool Node::isNonTerminal() const {
    return kind == NodeKind::NonTerminal;
}

bool Node::isError() const {
    return kind == NodeKind::Error;
}

const std::string& Node::getLabel() const {
    return label;
}

const std::string& Node::getValue() const {
    return value;
}

int Node::getLine() const {
    return line;
}

int Node::getColumn() const {
    return column;
}

const std::vector<std::unique_ptr<Node>>& Node::getChildren() const {
    return children;
}

std::string Node::toString() const {
    if (isNonTerminal()) {
        return "<" + label + ">";
    }

    if (isError()) {
        return "<error: " + value + ">";
    }

    if (!value.empty()) {
        return label + " (" + value + ")";
    }
    return label;
}

void Node::print(std::ostream& out, int depth) const {
    for (int i = 0; i < depth; ++i) {
        out << "  ";
    }

    out << toString();
    if (line >= 0 && column >= 0) {
        out << " [" << line << ":" << column << "]";
    }
    out << '\n';

    for (const auto& child : children) {
        child->print(out, depth + 1);
    }
}

const char* toString(NonTerminal symbol) {
    switch (symbol) {
    case NonTerminal::Program: return "program";
    case NonTerminal::ProgramHeader: return "program-header";
    case NonTerminal::IdentifierList: return "identifier-list";
    case NonTerminal::Block: return "block";
    case NonTerminal::DeclarationPart: return "declaration-part";
    case NonTerminal::ConstDeclaration: return "const-declaration";
    case NonTerminal::ConstDefinition: return "const-definition";
    case NonTerminal::TypeDeclaration: return "type-declaration";
    case NonTerminal::TypeDefinition: return "type-definition";
    case NonTerminal::VarDeclaration: return "var-declaration";
    case NonTerminal::VariableDeclaration: return "variable-declaration";
    case NonTerminal::Type: return "type";
    case NonTerminal::ArrayType: return "array-type";
    case NonTerminal::Range: return "range";
    case NonTerminal::Enumerated: return "enumerated";
    case NonTerminal::RecordType: return "record-type";
    case NonTerminal::FieldList: return "field-list";
    case NonTerminal::SubprogramDeclarationPart: return "subprogram-declaration-part";
    case NonTerminal::SubprogramDeclaration: return "subprogram-declaration";
    case NonTerminal::ProcedureDeclaration: return "procedure-declaration";
    case NonTerminal::FunctionDeclaration: return "function-declaration";
    case NonTerminal::ProcedureHeading: return "procedure-heading";
    case NonTerminal::FunctionHeading: return "function-heading";
    case NonTerminal::FormalParameterList: return "formal-parameter-list";
    case NonTerminal::ParameterGroup: return "parameter-group";
    case NonTerminal::CompoundStatement: return "compound-statement";
    case NonTerminal::StatementList: return "statement-list";
    case NonTerminal::Statement: return "statement";
    case NonTerminal::AssignmentStatement: return "assignment-statement";
    case NonTerminal::ProcedureFunctionCall: return "procedure/function-call";
    case NonTerminal::IfStatement: return "if-statement";
    case NonTerminal::CaseStatement: return "case-statement";
    case NonTerminal::CaseBlock: return "case-block";
    case NonTerminal::RepeatStatement: return "repeat-statement";
    case NonTerminal::WhileStatement: return "while-statement";
    case NonTerminal::ForStatement: return "for-statement";
    case NonTerminal::ParameterList: return "parameter-list";
    case NonTerminal::Expression: return "expression";
    case NonTerminal::RelationalOperator: return "relational-operator";
    case NonTerminal::SimpleExpression: return "simple-expression";
    case NonTerminal::AdditiveOperator: return "additive-operator";
    case NonTerminal::Term: return "term";
    case NonTerminal::MultiplicativeOperator: return "multiplicative-operator";
    case NonTerminal::Factor: return "factor";
    case NonTerminal::Variable: return "variable";
    case NonTerminal::ComponentVariable: return "component-variable";
    case NonTerminal::Selector: return "selector";
    case NonTerminal::IndexList: return "index-list";
    case NonTerminal::Constant: return "constant";
    case NonTerminal::Empty: return "empty";
    }

    throw std::logic_error("Unhandled non-terminal symbol");
}

NodePtr makeNonTerminal(NonTerminal symbol) {
    return std::make_unique<Node>(NodeKind::NonTerminal, toString(symbol));
}

NodePtr makeNonTerminal(const std::string& label) {
    return std::make_unique<Node>(NodeKind::NonTerminal, label);
}

NodePtr makeTerminal(const std::string& tokenType, const std::string& lexeme, int line, int column) {
    return std::make_unique<Node>(NodeKind::Terminal, tokenType, lexeme, line, column);
}

NodePtr makeTerminal(const Token& token) {
    return makeTerminal(token.get_type_name(), token.get_value());
}

NodePtr makeError(const std::string& message, int line, int column) {
    return std::make_unique<Node>(NodeKind::Error, "error", message, line, column);
}

Node* createNonTerminal(NonTerminal symbol) {
    return makeNonTerminal(symbol).release();
}

Node* createNonTerminal(const std::string& label) {
    return makeNonTerminal(label).release();
}

Node* createTerminal(const std::string& tokenType, const std::string& lexeme, int line, int column) {
    return makeTerminal(tokenType, lexeme, line, column).release();
}

Node* createTerminal(const Token& token) {
    return makeTerminal(token).release();
}

Node* createError(const std::string& message, int line, int column) {
    return makeError(message, line, column).release();
}

void printTree(const Node* root, std::ostream& out) {
    if (root == nullptr) {
        out << "<empty tree>\n";
        return;
    }
    root->print(out);
}

} // namespace parse_tree
