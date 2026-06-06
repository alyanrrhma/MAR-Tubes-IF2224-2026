#pragma once
#include "symbol_table.hpp"
#include "ast_nodes.hpp"

#include <string>
#include <unordered_set>
#include <vector>

class TypeChecker {
public:
    TypeChecker();

    void check(semantic::AstNode* root, semantic::SymbolTable& symTab);

    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

    void printErrors(std::ostream& out) const;

private:
    semantic::SymbolTable* sym_ = nullptr; 
    std::vector<std::string> errors_;

    int loopDepth_ = 0;
    int assignmentTargetDepth_ = 0;
    bool suppressGlobalInitCheck_ = false;
    std::unordered_set<int> initialized_;
    int currentFunctionIdx_ = semantic::NO_INDEX;

    void visit(semantic::AstNode* node);

    void visitProgram(semantic::ProgramNode& n);
    void visitDeclarations(semantic::DeclarationNode& n);
    void visitBlock(semantic::BlockNode& n);
    void visitConstDecl(semantic::ConstDeclNode& n);
    void visitTypeDecl(semantic::TypeDeclNode& n);
    void visitVarDecl(semantic::VarDeclNode& n);
    void visitProcDecl(semantic::ProcDeclNode& n);
    void visitFuncDecl(semantic::FuncDeclNode& n);

    void visitAssign(semantic::AssignNode& n);
    void visitReturn(semantic::ReturnNode& n);
    void visitProcCall(semantic::ProcCallNode& n);
    void visitIf(semantic::IfNode& n);
    void visitCase(semantic::CaseNode& n);
    void visitCaseBranch(semantic::CaseBranchNode& n);
    void visitWhile(semantic::WhileNode& n);
    void visitRepeat(semantic::RepeatNode& n);
    void visitFor(semantic::ForNode& n);
    void visitCompound(semantic::CompoundNode& n);

    void visitBinOp(semantic::BinOpNode& n);
    void visitUnaryOp(semantic::UnaryOpNode& n);
    void visitVar(semantic::VarNode& n);
    void visitArrayAccess(semantic::ArrayAccessNode& n);
    void visitFieldAccess(semantic::FieldAccessNode& n);

    void visitIntLit(semantic::IntLitNode& n);
    void visitRealLit(semantic::RealLitNode& n);
    void visitCharLit(semantic::CharLitNode& n);
    void visitStringLit(semantic::StringLitNode& n);
    void visitBoolLit(semantic::BoolLitNode& n);

    
    bool compatible(semantic::TypeKind a, semantic::TypeKind b) const;

    
    bool compatibleWithRef(semantic::TypeKind a, int aRef,
                           semantic::TypeKind b, int bRef) const;

    bool sameArrayTypeRef(int aRef, int bRef) const;
    bool sameArrayEntry(int aRef, int bRef, std::unordered_set<long long>& seen) const;

    bool assignmentCompatible(semantic::TypeKind lhsType, int lhsRef,
                              semantic::TypeKind rhsType, int rhsRef) const;

    bool isAssignableObject(semantic::ObjectKind obj) const;
    int assignmentBaseIndex(const semantic::AstNode* node) const;

    void checkSubrangeBounds(const semantic::AstNode& node,
                              int lhsRef, const semantic::AstNode* rhsNode) const;

    bool isArithmetic(semantic::TypeKind t) const;

    bool isRelational(semantic::TypeKind t) const;

    static std::string typeName(semantic::TypeKind t);

    bool isInitialized(int tabIdx) const;
    void markInitialized(semantic::AstNode* node);
    void seedInitiallyInitialized();
    bool constantValue(const semantic::AstNode* node, long long& value) const;

    bool isFunctionReturnAssign(const semantic::AssignNode& node, int functionIdx) const;
    bool statementMustReturn(const semantic::AstNode* node, int functionIdx) const;
    bool compoundMustReturn(const semantic::CompoundNode& node, int functionIdx) const;
    bool ifMustReturn(const semantic::IfNode& node, int functionIdx) const;
    bool caseMustReturn(const semantic::CaseNode& node, int functionIdx) const;
    bool repeatMustReturn(const semantic::RepeatNode& node, int functionIdx) const;

    void report(const semantic::AstNode& node, const std::string& msg);
    void reportTypeMismatch(const semantic::AstNode& node,
                            const std::string& context,
                            semantic::TypeKind expected,
                            semantic::TypeKind got);
};