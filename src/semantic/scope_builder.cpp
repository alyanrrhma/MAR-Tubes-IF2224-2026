#include "scope_builder.hpp"

const semantic::SymbolTable& ScopeBuilder::symbolTable() const {return *symTab;}
semantic::SymbolTable& ScopeBuilder::symbolTable() {return *symTab;}
const std::vector<std::string>& ScopeBuilder::errors() const {return errorMsg;}
bool ScopeBuilder::hasErrors() const {return !errorMsg.empty();}
void ScopeBuilder::visit(semantic::AstPtr& astPtr) {visit(astPtr.get());}

ScopeBuilder::ScopeBuilder() : symTab(std::make_unique<semantic::SymbolTable>()) {}

void ScopeBuilder::build(semantic::AstPtr& astPtr)
{
    symTab = std::make_unique<semantic::SymbolTable>();
    symTab->initPredefined();
    errorMsg.clear();
    visit(astPtr);
}

void ScopeBuilder::printTables(std::ostream& out) const { symTab->printAll(out);}

void ScopeBuilder::printErrors(std::ostream& out) const
{
    for (const auto& error: errorMsg)
    {
        out << error << '\n';
    }
}

void ScopeBuilder::visitDeclarationNode(semantic::DeclarationNode& node) {
    for (auto& decl : node.declarations) {
        visit(decl);
    }
}

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
    node.level = symTab->currentLevel();
    node.tabIdx = declareIdentifier(node, node.name, semantic::ObjectKind::Program, {semantic::TypeKind::Void, symTab->currentBlock(), 0});
}

void ScopeBuilder::visitConstDeclNode(semantic::ConstDeclNode& node)
{
    TypeInfo type = inferExpression(node.value.get());
    node.inferredType = type.type;
    node.typeRef = type.ref;
    node.tabIdx = declareIdentifier(node, node.name, semantic::ObjectKind::Constant, type);
}
void ScopeBuilder::visitTypeDeclNode(semantic::TypeDeclNode& node)
{
    TypeInfo type = resolveTypeExpr(node.typeExpr.get());
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