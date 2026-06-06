#ifndef SEMANTIC_AST_NODES_HPP
#define SEMANTIC_AST_NODES_HPP

#include "symbol_table.hpp"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace semantic {

enum class AstKind {
    Program,
    DeclPart,
    Block,
    ConstDecl,
    TypeDecl,
    VarDecl,
    ProcDecl,
    FuncDecl,
    SimpleType,
    ArrayType,
    RecordType,
    Range,
    EnumeratedType,
    Assign,
    Return,
    ProcCall,
    If,
    Case,
    CaseBranch,
    While,
    Repeat,
    For,
    Compound,
    BinOp,
    UnaryOp,
    Var,
    IntLit,
    RealLit,
    CharLit,
    StringLit,
    BoolLit,
    ArrayAccess,
    FieldAccess
};

const char* toString(AstKind kind);

enum class BinOpKind {
    Add,
    Sub,
    Or,
    Mul,
    Div,
    IntDiv,
    Mod,
    And,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge
};

const char* toString(BinOpKind op);

enum class UnaryOpKind {
    Plus,
    Minus,
    Not
};

const char* toString(UnaryOpKind op);

class AstNode {
public:
    explicit AstNode(AstKind kind);
    virtual ~AstNode();

    AstKind getKind() const { return kind_; }
    const char* kindName() const { return toString(kind_); }

    TypeKind inferredType = TypeKind::Unknown;
    int typeRef = NO_INDEX;
    int tabIdx = NO_INDEX;
    int level = NO_INDEX;
    int line = NO_INDEX;
    int column = NO_INDEX;

    virtual void print(std::ostream& out, int depth = 0) const;

protected:
    void printIndent(std::ostream& out, int depth) const;
    void printAnnotations(std::ostream& out) const;
    virtual void printSelf(std::ostream& out) const;
    virtual void printChildren(std::ostream& out, int depth) const;

private:
    AstKind kind_;
};

using AstPtr = std::unique_ptr<AstNode>;

void printAst(std::ostream& out, const AstNode* root);

class DeclarationNode : public AstNode {
public:
    DeclarationNode();
    std::vector<AstPtr> declarations; 

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class CompoundNode;

class BlockNode : public AstNode {
public:
    BlockNode();

    std::unique_ptr<DeclarationNode> declaration;
    std::unique_ptr<CompoundNode> statements;
    
    protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class ProgramNode : public AstNode {
public:
    ProgramNode();

    std::string name;
    std::unique_ptr<DeclarationNode> declaration;
    std::unique_ptr<CompoundNode> statements;
    
protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class ConstDeclNode : public AstNode {
public:
    ConstDeclNode();

    std::string name;
    AstPtr value;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class TypeDeclNode : public AstNode {
public:
    TypeDeclNode();

    std::string name;
    AstPtr typeExpr;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class VarDeclNode : public AstNode {
public:
    VarDeclNode();

    std::string name;
    AstPtr typeExpr;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

struct FormalParam {
    std::string name;
    AstPtr typeExpr;
    bool byReference = false;
};

class ProcDeclNode : public AstNode {
public:
    ProcDeclNode();

    std::string name;
    std::vector<FormalParam> params;
    std::unique_ptr<BlockNode> block;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class FuncDeclNode : public AstNode {
public:
    FuncDeclNode();

    std::string name;
    std::vector<FormalParam> params;
    AstPtr returnType;
    std::unique_ptr<BlockNode> block;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class SimpleTypeNode : public AstNode {
public:
    SimpleTypeNode();

    std::string name;

protected:
    void printSelf(std::ostream& out) const override;
};

class RangeNode : public AstNode {
public:
    RangeNode();

    AstPtr low;
    AstPtr high;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class ArrayTypeNode : public AstNode {
public:
    ArrayTypeNode();

    std::vector<AstPtr> indexTypes;
    AstPtr elementType;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class RecordTypeNode : public AstNode {
public:
    struct FieldSection {
        std::vector<std::string> names;
        AstPtr typeExpr;
    };

    RecordTypeNode();
    std::vector<FieldSection> fields;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class EnumeratedTypeNode : public AstNode {
public:
    EnumeratedTypeNode();

    std::vector<std::string> identifiers;

protected:
    void printSelf(std::ostream& out) const override;
};

class AssignNode : public AstNode {
public:
    AssignNode();

    AstPtr target;
    AstPtr value;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class ReturnNode : public AstNode {
public:
    ReturnNode();

    AstPtr value;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class ProcCallNode : public AstNode {
public:
    ProcCallNode();

    std::string name;
    std::vector<AstPtr> args;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class IfNode : public AstNode {
public:
    IfNode();

    AstPtr condition;
    AstPtr thenStmt;
    AstPtr elseStmt;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class CaseBranchNode : public AstNode {
public:
    CaseBranchNode();

    std::vector<AstPtr> labels;
    AstPtr statement;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class CaseNode : public AstNode {
public:
    CaseNode();

    AstPtr selector;
    std::vector<std::unique_ptr<CaseBranchNode>> branches;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class WhileNode : public AstNode {
public:
    WhileNode();

    AstPtr condition;
    AstPtr body;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class RepeatNode : public AstNode {
public:
    RepeatNode();

    std::unique_ptr<CompoundNode> body;
    AstPtr condition;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class ForNode : public AstNode {
public:
    ForNode();

    std::string controlVar;
    AstPtr startExpr;
    AstPtr endExpr;
    AstPtr body;
    bool downto = false;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class CompoundNode : public AstNode {
public:
    CompoundNode();

    std::vector<AstPtr> statements;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class BinOpNode : public AstNode {
public:
    BinOpNode();

    BinOpKind op = BinOpKind::Add;
    AstPtr lhs;
    AstPtr rhs;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class UnaryOpNode : public AstNode {
public:
    UnaryOpNode();

    UnaryOpKind op = UnaryOpKind::Plus;
    AstPtr operand;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

class VarNode : public AstNode {
public:
    VarNode();

    std::string name;

protected:
    void printSelf(std::ostream& out) const override;
};

class IntLitNode : public AstNode {
public:
    IntLitNode();

    long long value = 0;

protected:
    void printSelf(std::ostream& out) const override;
};

class RealLitNode : public AstNode {
public:
    RealLitNode();

    double value = 0.0;

protected:
    void printSelf(std::ostream& out) const override;
};

class CharLitNode : public AstNode {
public:
    CharLitNode();

    char value = '\0';

protected:
    void printSelf(std::ostream& out) const override;
};

class StringLitNode : public AstNode {
public:
    StringLitNode();

    std::string value;

protected:
    void printSelf(std::ostream& out) const override;
};

class BoolLitNode : public AstNode {
public:
    BoolLitNode();

    bool value = false;

protected:
    void printSelf(std::ostream& out) const override;
};

class ArrayAccessNode : public AstNode {
public:
    ArrayAccessNode();

    AstPtr base;
    std::vector<AstPtr> indices;

protected:
    void printChildren(std::ostream& out, int depth) const override;
};

class FieldAccessNode : public AstNode {
public:
    FieldAccessNode();

    AstPtr base;
    std::string field;

protected:
    void printSelf(std::ostream& out) const override;
    void printChildren(std::ostream& out, int depth) const override;
};

}

#endif
