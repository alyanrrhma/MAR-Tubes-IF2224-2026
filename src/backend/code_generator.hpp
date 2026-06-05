#ifndef BACKEND_CODE_GENERATOR_HPP
#define BACKEND_CODE_GENERATOR_HPP

#include "instruction.hpp"
#include "../semantic/ast_nodes.hpp"
#include "../semantic/symbol_table.hpp"

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

    void generateNode(const semantic::AstNode* node);
    void generateStatement(const semantic::AstNode* node);
    void generateExpression(const semantic::AstNode* node);
    void generateStore(const semantic::AstNode* target);

    void generateProgram(const semantic::ProgramNode& node);
    void generateBlock(const semantic::BlockNode& node);
    void generateCompound(const semantic::CompoundNode& node);
    void generateAssign(const semantic::AssignNode& node);
    void generateProcCall(const semantic::ProcCallNode& node);
    void generateIf(const semantic::IfNode& node);
    void generateWhile(const semantic::WhileNode& node);
    void generateVar(const semantic::VarNode& node);
    void generateIntLit(const semantic::IntLitNode& node);
    void generateBoolLit(const semantic::BoolLitNode& node);
    void generateUnaryOp(const semantic::UnaryOpNode& node);
    void generateBinOp(const semantic::BinOpNode& node);

    int variableAddress(const semantic::AstNode& node) const;
    int levelDifference(const semantic::AstNode& node) const;
    int initialFrameSize(const semantic::ProgramNode& node) const;
    static OprCode mapBinaryOp(semantic::BinOpKind op);
};

}  

#endif
