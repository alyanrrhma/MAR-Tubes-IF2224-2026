#pragma once
#include "symbol_table.hpp"
#include "ast_nodes.hpp"
#include "vector"
#include "string"

class ScopeBuilder
{
private:
    std::unique_ptr<semantic::SymbolTable> symTab;
    std::vector<std::string> errorMsg;
    void visitBlockNode(semantic::BlockNode& astPtr);
    void visitProgramNode(semantic::ProgramNode& astPtr);
    void visitConstDeclNode(semantic::ConstDeclNode& astPtr);
    void visitTypeDeclNode(semantic::TypeDeclNode& astPtr);
    void visitVarDeclNode(semantic::VarDeclNode& astPtr);
    void visitFormalParam(semantic::FormalParam& astPtr);
    void visitProcDeclNode(semantic::ProcDeclNode& astPtr);
    void visitFuncDeclNode(semantic::FuncDeclNode& astPtr);
    void visitSimpleTypeNode(semantic::SimpleTypeNode& astPtr);
    void visitRangeNode(semantic::RangeNode& astPtr);
    void visitArrayTypeNode(semantic::ArrayTypeNode& astPtr);
    void visitRecordTypeNode(semantic::RecordTypeNode& astPtr);
    void visitEnumeratedTypeNode(semantic::EnumeratedTypeNode& astPtr);
    void visitAssignNode(semantic::AssignNode& astPtr);
    void visitProcCallNode(semantic::ProcCallNode& astPtr);
    void visitIfNode(semantic::IfNode& astPtr);
    void visitCaseBranchNode(semantic::CaseBranchNode& astPtr);
    void visitCaseNode(semantic::CaseNode& astPtr);
    void visitWhileNode(semantic::WhileNode& astPtr);
    void visitRepeatNode(semantic::RepeatNode& astPtr);
    void visitForNode(semantic::ForNode& astPtr);
    void visitCompoundNode(semantic::CompoundNode& astPtr);
    void visitBinOpNode(semantic::BinOpNode& astPtr);
    void visitUnaryOpNode(semantic::UnaryOpNode& astPtr);
    void visitVarNode(semantic::VarNode& astPtr);
    void visitIntLitNode(semantic::IntLitNode& astPtr);
    void visitRealLitNode(semantic::RealLitNode& astPtr);
    void visitCharLitNode(semantic::CharLitNode& astPtr);
    void visitStringLitNode(semantic::StringLitNode& astPtr);
    void visitBoolLitNode(semantic::BoolLitNode& astPtr);
    void visitArrayAccessNode(semantic::ArrayAccessNode& astPtr);
    void visitFieldAccessNode(semantic::FieldAccessNode& astPtr);
public:
    explicit ScopeBuilder();
    void build(semantic::AstPtr& astPtr);
};