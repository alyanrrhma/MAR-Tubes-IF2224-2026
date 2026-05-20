#include "scope_builder.hpp"

const semantic::SymbolTable& ScopeBuilder::symbolTable() const {return *symTab;}
semantic::SymbolTable& ScopeBuilder::symbolTable() {return *symTab;}
const std::vector<std::string>& ScopeBuilder::errors() const {return errorMsg;}
bool ScopeBuilder::hasErrors() const {return !errorMsg.empty();}

void ScopeBuilder::visit(semantic::AstPtr& astPtr) {visit(astPtr.get());}


void ScopeBuilder::visitBlockNode(semantic::BlockNode& node, bool ownScope)
{
    if (ownScope)
    {
        symTab->pushBlock();
        node.level = symTab->currentLevel();
    }

    visit(node.declaration.get());
    visit(node.statements.get());
    
    if (ownScope) symTab->popBlock();
}
void ScopeBuilder::visitProgramNode(semantic::ProgramNode& node)
{

}
void ScopeBuilder::visitConstDeclNode(semantic::ConstDeclNode& node)
{

}
void ScopeBuilder::visitTypeDeclNode(semantic::TypeDeclNode& node)
{

}
void ScopeBuilder::visitVarDeclNode(semantic::VarDeclNode& node)
{

}
void ScopeBuilder::visitFormalParam(semantic::FormalParam& param)
{

}
void ScopeBuilder::visitProcDeclNode(semantic::ProcDeclNode& node)
{

}
void ScopeBuilder::visitFuncDeclNode(semantic::FuncDeclNode& node)
{

}
void ScopeBuilder::visitSimpleTypeNode(semantic::SimpleTypeNode& node)
{

}
void ScopeBuilder::visitRangeNode(semantic::RangeNode& node)
{

}
void ScopeBuilder::visitArrayTypeNode(semantic::ArrayTypeNode& node)
{

}
void ScopeBuilder::visitRecordTypeNode(semantic::RecordTypeNode& node)
{

}
void ScopeBuilder::visitEnumeratedTypeNode(semantic::EnumeratedTypeNode& node)
{

}
void ScopeBuilder::visitAssignNode(semantic::AssignNode& node)
{

}
void ScopeBuilder::visitProcCallNode(semantic::ProcCallNode& node)
{

}
void ScopeBuilder::visitIfNode(semantic::IfNode& node)
{

}
void ScopeBuilder::visitCaseBranchNode(semantic::CaseBranchNode& node)
{

}
void ScopeBuilder::visitCaseNode(semantic::CaseNode& node)
{

}
void ScopeBuilder::visitWhileNode(semantic::WhileNode& node)
{

}
void ScopeBuilder::visitRepeatNode(semantic::RepeatNode& node)
{

}
void ScopeBuilder::visitForNode(semantic::ForNode& node)
{

}
void ScopeBuilder::visitCompoundNode(semantic::CompoundNode& node)
{

}
void ScopeBuilder::visitBinOpNode(semantic::BinOpNode& node)
{

}
void ScopeBuilder::visitUnaryOpNode(semantic::UnaryOpNode& node)
{

}
void ScopeBuilder::visitVarNode(semantic::VarNode& node)
{

}
void ScopeBuilder::visitIntLitNode(semantic::IntLitNode& node)
{

}
void ScopeBuilder::visitRealLitNode(semantic::RealLitNode& node)
{

}
void ScopeBuilder::visitCharLitNode(semantic::CharLitNode& node)
{

}
void ScopeBuilder::visitStringLitNode(semantic::StringLitNode& node)
{

}
void ScopeBuilder::visitBoolLitNode(semantic::BoolLitNode& node)
{

}
void ScopeBuilder::visitArrayAccessNode(semantic::ArrayAccessNode& node)
{

}
void ScopeBuilder::visitFieldAccessNode(semantic::FieldAccessNode& node)
{

}