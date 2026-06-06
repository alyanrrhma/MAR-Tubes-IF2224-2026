#pragma once
#include "symbol_table.hpp"
#include "ast_nodes.hpp"
#include <vector>
#include <iostream>

class ScopeBuilder
{
private:
    // L-attributed semantic flow used by ScopeBuilder:
    // - inherited attribute: active block/scope is carried by symTab scope stack
    // - synthesized attributes: TypeInfo {type, ref, size} is returned upward
    //   from type/expression nodes and stored back into the decorated AST
    struct TypeInfo {
        semantic::TypeKind type = semantic::TypeKind::Unknown;
        int ref = semantic::NO_INDEX;
        int size = 1;
    };
    std::unique_ptr<semantic::SymbolTable> symTab;
    std::vector<std::string> errorMsg;

    void visit(semantic::AstNode* node);
    void visit(semantic::AstPtr& astPtr);

    void visitDeclarationNode(semantic::DeclarationNode& node);
    void visitBlockNode(semantic::BlockNode& node, bool ownScope);
    void visitProgramNode(semantic::ProgramNode& node);
    void visitConstDeclNode(semantic::ConstDeclNode& node);
    void visitTypeDeclNode(semantic::TypeDeclNode& node);
    void visitVarDeclNode(semantic::VarDeclNode& node);
    void visitFormalParam(semantic::FormalParam& param);
    void visitProcDeclNode(semantic::ProcDeclNode& node);
    void visitFuncDeclNode(semantic::FuncDeclNode& node);
    void visitSimpleTypeNode(semantic::SimpleTypeNode& node);
    void visitRangeNode(semantic::RangeNode& node);
    void visitArrayTypeNode(semantic::ArrayTypeNode& node);
    void visitRecordTypeNode(semantic::RecordTypeNode& node);
    void visitEnumeratedTypeNode(semantic::EnumeratedTypeNode& node);
    void visitAssignNode(semantic::AssignNode& node);
    void visitReturnNode(semantic::ReturnNode& node);
    void visitProcCallNode(semantic::ProcCallNode& node);
    void visitIfNode(semantic::IfNode& node);
    void visitCaseBranchNode(semantic::CaseBranchNode& node);
    void visitCaseNode(semantic::CaseNode& node);
    void visitWhileNode(semantic::WhileNode& node);
    void visitRepeatNode(semantic::RepeatNode& node);
    void visitForNode(semantic::ForNode& node);
    void visitCompoundNode(semantic::CompoundNode& node);
    void visitBinOpNode(semantic::BinOpNode& node);
    void visitUnaryOpNode(semantic::UnaryOpNode& node);
    void visitVarNode(semantic::VarNode& node);
    void visitIntLitNode(semantic::IntLitNode& node);
    void visitRealLitNode(semantic::RealLitNode& node);
    void visitCharLitNode(semantic::CharLitNode& node);
    void visitStringLitNode(semantic::StringLitNode& node);
    void visitBoolLitNode(semantic::BoolLitNode& node);
    void visitArrayAccessNode(semantic::ArrayAccessNode& node);
    void visitFieldAccessNode(semantic::FieldAccessNode& node);

    TypeInfo resolveTypeExpr(semantic::AstNode* node);
    TypeInfo resolveSimpleType(semantic::SimpleTypeNode& node);
    TypeInfo resolveArrayType(semantic::ArrayTypeNode& node);
    TypeInfo resolveRecordType(semantic::RecordTypeNode& node);
    TypeInfo resolveRangeType(semantic::RangeNode& node);
    TypeInfo resolveEnumeratedType(semantic::EnumeratedTypeNode& node);

    TypeInfo inferExpression(semantic::AstNode* node);
    long long constIntValue(semantic::AstNode* node, bool& ok);

    int declareIdentifier(
        semantic::AstNode& owner, const std::string& name,
        semantic::ObjectKind obj,
        TypeInfo type,
        int adr = 0,
        bool nrm = true
    );

    int lookupIdentifier(semantic::AstNode& owner, const std::string& name);

    int allocateAddress(int size);
    void report(const semantic::AstNode& node, const std::string& message);

    bool sameName(const std::string& lhs, const std::string& rhs);
    int primitiveSize(semantic::TypeKind type);

public:
    explicit ScopeBuilder();
    void build(semantic::AstPtr& astPtr);
    const semantic::SymbolTable& symbolTable() const;
    semantic::SymbolTable& symbolTable();
    const std::vector<std::string>& errors() const;
    bool hasErrors() const;
    void printTables(std::ostream& out) const;
    void printErrors(std::ostream& out) const;
};