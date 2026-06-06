#ifndef BACKEND_CODE_GENERATOR_HPP
#define BACKEND_CODE_GENERATOR_HPP

#include "instruction.hpp"
#include "../semantic/ast_nodes.hpp"
#include "../semantic/symbol_table.hpp"

#include <unordered_map>
#include <vector>

namespace backend {

class CodeGenerator {
public:
    CodeGenerator();

    InstructionProgram generate(const semantic::AstNode* root,
                                const semantic::SymbolTable& symbolTable);

private:
    static constexpr int FRAME_HEADER_SIZE = 3;

    const semantic::SymbolTable* symbolTable_ = nullptr;
    InstructionProgram program_;
    int currentLevel_ = 0;

    // tabIdx → instruction address of proc/func entry point
    std::unordered_map<int, int> procAddresses_;
    // level → psze of the proc/func declared at that level
    std::vector<int> procPsizeByLevel_;

    void generateNode(const semantic::AstNode* node);
    void generateStatement(const semantic::AstNode* node);
    void generateExpression(const semantic::AstNode* node);
    void generateStore(const semantic::AstNode* target);

    void generateProgram(const semantic::ProgramNode& node);
    void generateBlock(const semantic::BlockNode& node);
    void generateCompound(const semantic::CompoundNode& node);
    void generateAssign(const semantic::AssignNode& node);
    void generateProcCall(const semantic::ProcCallNode& node);
    void generateProcDecl(const semantic::ProcDeclNode& node);
    void generateFuncDecl(const semantic::FuncDeclNode& node);
    void generateReturn(const semantic::ReturnNode& node);
    void generateIf(const semantic::IfNode& node);
    void generateWhile(const semantic::WhileNode& node);
    void generateVar(const semantic::VarNode& node);
    void generateIntLit(const semantic::IntLitNode& node);
    void generateBoolLit(const semantic::BoolLitNode& node);
    void generateUnaryOp(const semantic::UnaryOpNode& node);
    void generateBinOp(const semantic::BinOpNode& node);
    void generateStringLit(const semantic::StringLitNode& node);
    void generateArrayAccess(const semantic::ArrayAccessNode& node);
    void generateFieldAccess(const semantic::FieldAccessNode& node);

    // Pushes the absolute stack address of an lvalue onto the evaluation stack
    void generateAddress(const semantic::AstNode* node);

    // Generates nested proc/func declarations found in a declaration part
    void generateNestedDeclarations(const semantic::DeclarationNode& decls);

    int variableAddress(const semantic::AstNode& node) const;
    int levelDifference(const semantic::AstNode& node) const;
    int initialFrameSize(const semantic::ProgramNode& node) const;
    int procFrameSize(int btabIdx) const;
    static OprCode mapBinaryOp(semantic::BinOpKind op);
};

}  

#endif
