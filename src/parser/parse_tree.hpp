#ifndef PARSE_TREE_HPP
#define PARSE_TREE_HPP

#include "../lexer/token.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace parse_tree {

enum class NodeKind {
    Terminal,
    NonTerminal,
    Error
};

enum class NonTerminal {
    Program,
    ProgramHeader,
    ProgramHeading = ProgramHeader,
    IdentifierList,
    Block,
    DeclarationPart,
    ConstDeclaration,
    ConstantDeclaration = ConstDeclaration,
    ConstDefinition,
    ConstantDefinition = ConstDefinition,
    TypeDeclaration,
    TypeDefinition,
    VarDeclaration,
    VariableDeclaration,
    Type,
    TypeDenoter = Type,
    ArrayType,
    Range,
    Enumerated,
    RecordType,
    FieldList,
    FieldPart,
    SubprogramDeclarationPart,
    SubprogramDeclaration,
    ProcedureDeclaration,
    FunctionDeclaration,
    ProcedureHeading,
    FunctionHeading,
    FormalParameterList,
    ParameterGroup,
    FormalParameterSection = ParameterGroup,
    CompoundStatement,
    StatementList,
    Statement,
    AssignmentStatement,
    ProcedureFunctionCall,
    ProcedureCallStatement = ProcedureFunctionCall,
    IfStatement,
    CaseStatement,
    CaseBlock,
    RepeatStatement,
    WhileStatement,
    ForStatement,
    ParameterList,
    Expression,
    RelationalOperator,
    SimpleExpression,
    AdditiveOperator,
    Term,
    MultiplicativeOperator,
    Factor,
    Variable,
    ComponentVariable,
    Selector,
    IndexList,
    Constant,
    Empty
};

class Node {
public:
    Node(NodeKind kind, std::string label, std::string value = "", int line = -1, int column = -1);

    Node* addChild(Node* child);
    Node* addChild(std::unique_ptr<Node> child);

    NodeKind getKind() const;
    bool isTerminal() const;
    bool isNonTerminal() const;
    bool isError() const;
    const std::string& getLabel() const;
    const std::string& getValue() const;
    int getLine() const;
    int getColumn() const;
    const std::vector<std::unique_ptr<Node>>& getChildren() const;

    std::string toString() const;
    void print(std::ostream& out, int depth = 0) const;

private:
    NodeKind kind;
    std::string label;
    std::string value;
    int line;
    int column;
    std::vector<std::unique_ptr<Node>> children;
};

using NodePtr = std::unique_ptr<Node>;

const char* toString(NonTerminal symbol);

NodePtr makeNonTerminal(NonTerminal symbol);
NodePtr makeNonTerminal(const std::string& label);
NodePtr makeTerminal(const std::string& tokenType, const std::string& lexeme = "", int line = -1, int column = -1);
NodePtr makeTerminal(const Token& token);
NodePtr makeError(const std::string& message, int line = -1, int column = -1);

Node* createNonTerminal(NonTerminal symbol);
Node* createNonTerminal(const std::string& label);
Node* createTerminal(const std::string& tokenType, const std::string& lexeme = "", int line = -1, int column = -1);
Node* createTerminal(const Token& token);
Node* createError(const std::string& message, int line = -1, int column = -1);

void printTree(const Node* root, std::ostream& out);

} // namespace parse_tree

#endif
