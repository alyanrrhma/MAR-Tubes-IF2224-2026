#include "scope_builder.hpp"


void ScopeBuilder::visitBlockNode(semantic::BlockNode& astPtr)
{
    symTab->pushBlock();
    node.level = symTab->currentLevel();

    visit(node.declaration.get());
    visit(node.statements.get());

    symTab->popBlock();
}
void ScopeBuilder::visitProgramNode(semantic::ProgramNode& astPtr)
{

}
void ScopeBuilder::visitConstDeclNode(semantic::ConstDeclNode& astPtr)
{

}
void ScopeBuilder::visitTypeDeclNode(semantic::TypeDeclNode& astPtr)
{

}
void ScopeBuilder::visitVarDeclNode(semantic::VarDeclNode& astPtr)
{

}
void ScopeBuilder::visitFormalParam(semantic::FormalParam& astPtr)
{

}
void ScopeBuilder::visitProcDeclNode(semantic::ProcDeclNode& astPtr)
{

}
void ScopeBuilder::visitFuncDeclNode(semantic::FuncDeclNode& astPtr)
{

}
void ScopeBuilder::visitSimpleTypeNode(semantic::SimpleTypeNode& astPtr)
{

}
void ScopeBuilder::visitRangeNode(semantic::RangeNode& astPtr)
{

}
void ScopeBuilder::visitArrayTypeNode(semantic::ArrayTypeNode& astPtr)
{

}
void ScopeBuilder::visitRecordTypeNode(semantic::RecordTypeNode& astPtr)
{

}
void ScopeBuilder::visitEnumeratedTypeNode(semantic::EnumeratedTypeNode& astPtr)
{

}
void ScopeBuilder::visitAssignNode(semantic::AssignNode& astPtr)
{

}
void ScopeBuilder::visitProcCallNode(semantic::ProcCallNode& astPtr)
{

}
void ScopeBuilder::visitIfNode(semantic::IfNode& astPtr)
{

}
void ScopeBuilder::visitCaseBranchNode(semantic::CaseBranchNode& astPtr)
{

}
void ScopeBuilder::visitCaseNode(semantic::CaseNode& astPtr)
{

}
void ScopeBuilder::visitWhileNode(semantic::WhileNode& astPtr)
{

}
void ScopeBuilder::visitRepeatNode(semantic::RepeatNode& astPtr)
{

}
void ScopeBuilder::visitForNode(semantic::ForNode& astPtr)
{

}
void ScopeBuilder::visitCompoundNode(semantic::CompoundNode& astPtr)
{

}
void ScopeBuilder::visitBinOpNode(semantic::BinOpNode& astPtr)
{

}
void ScopeBuilder::visitUnaryOpNode(semantic::UnaryOpNode& astPtr)
{

}
void ScopeBuilder::visitVarNode(semantic::VarNode& astPtr)
{

}
void ScopeBuilder::visitIntLitNode(semantic::IntLitNode& astPtr)
{

}
void ScopeBuilder::visitRealLitNode(semantic::RealLitNode& astPtr)
{

}
void ScopeBuilder::visitCharLitNode(semantic::CharLitNode& astPtr)
{

}
void ScopeBuilder::visitStringLitNode(semantic::StringLitNode& astPtr)
{

}
void ScopeBuilder::visitBoolLitNode(semantic::BoolLitNode& astPtr)
{

}
void ScopeBuilder::visitArrayAccessNode(semantic::ArrayAccessNode& astPtr)
{

}
void ScopeBuilder::visitFieldAccessNode(semantic::FieldAccessNode& astPtr)
{

}