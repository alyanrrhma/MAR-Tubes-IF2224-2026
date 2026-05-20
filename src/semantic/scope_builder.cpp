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
    node.inferredType = type.type;
    node.typeRef = type.ref;
    node.tabIdx = declareIdentifier(node, node.name, semantic::ObjectKind::Type, type, type.size);
}
void ScopeBuilder::visitVarDeclNode(semantic::VarDeclNode& node)
{
    TypeInfo type = resolveTypeExpr(node.typeExpr.get());
    node.inferredType = type.type;
    node.typeRef = type.ref;

    const int adr = allocateAddress(type.size);
    node.tabIdx = declareIdentifier(node, node.name, semantic::ObjectKind::Variable, type, adr);
}
void ScopeBuilder::visitFormalParam(semantic::FormalParam& param)
{
    TypeInfo type = resolveTypeExpr(param.typeExpr.get());
    const int adr = allocateAddress(type.size);

    semantic::AstNode* owner = param.typeExpr.get();
    if(!owner) return;

    const int index = declareIdentifier(*owner, param.name, semantic::ObjectKind::Parameter, type, adr, !param.byReference);
    owner->tabIdx = index;
    owner->inferredType = type.type;
    owner->typeRef = type.ref;
}
void ScopeBuilder::visitProcDeclNode(semantic::ProcDeclNode& node)
{
    const int procIndex = declareIdentifier(node, node.name, semantic::ObjectKind::Procedure, {semantic::TypeKind::Void, semantic::NO_INDEX, 0});

    const int blockIndex = symTab->pushBlock();
    node.level = symTab->currentLevel();
    if (procIndex != semantic::NO_INDEX)
    {
        symTab->tabAt(procIndex).ref = blockIndex;
        node.typeRef = blockIndex;
    }
    for (auto& param: node.params) {visitFormalParam(param);}
    if (node.block)
    {
        visit(node.block->declaration.get());
        visit(node.block->statements.get());
    }
    symTab->popBlock();
}

void ScopeBuilder::visitFuncDeclNode(semantic::FuncDeclNode& node)
{
    TypeInfo returnType = resolveTypeExpr(node.returnType.get());
    node.inferredType = returnType.type;

    const int funcIndex = declareIdentifier(node, node.name, semantic::ObjectKind::Function, returnType);
    const int blockIndex = symTab->pushBlock();
    node.level = symTab->currentLevel();
    if (funcIndex != semantic::NO_INDEX)
    {
        symTab->tabAt(funcIndex).ref = blockIndex;
        node.typeRef = blockIndex;
    }

    for (auto& param: node.params) {visitFormalParam(param);}
    if (node.block)
    {
        visit(node.block->declaration.get());
        visit(node.block->statements.get());
    }
    symTab->popBlock();
}
void ScopeBuilder::visitSimpleTypeNode(semantic::SimpleTypeNode& node)
{
    TypeInfo type = resolveSimpleType(node);
    node.inferredType = type.type;
    node.typeRef = type.ref;
}

void ScopeBuilder::visitRangeNode(semantic::RangeNode& node)
{
    TypeInfo type = resolveRangeType(node);
    node.inferredType = type.type;
    node.typeRef = type.ref;
}

void ScopeBuilder::visitArrayTypeNode(semantic::ArrayTypeNode& node)
{
    TypeInfo type = resolveArrayType(node);
    node.inferredType = type.type;
    node.typeRef = type.ref;
}
void ScopeBuilder::visitRecordTypeNode(semantic::RecordTypeNode& node)
{
    TypeInfo type = resolveRecordType(node);
    node.inferredType = type.type;
    node.typeRef = type.ref;
}
void ScopeBuilder::visitEnumeratedTypeNode(semantic::EnumeratedTypeNode& node)
{
    TypeInfo type = resolveEnumeratedType(node);
    node.inferredType = type.type;
    node.typeRef = type.ref;
}
void ScopeBuilder::visitAssignNode(semantic::AssignNode& node)
{
    TypeInfo lhs = inferExpression(node.target.get());
    TypeInfo rhs = inferExpression(node.value.get());
    node.inferredType = rhs.type;
    node.typeRef = rhs.ref;
    if (node.target) node.tabIdx = node.target->tabIdx;
    (void) lhs;
}
void ScopeBuilder::visitProcCallNode(semantic::ProcCallNode& node)
{
    const int index = lookupIdentifier(node, node.name);
    if (index != semantic::NO_INDEX)
    {
        const auto& entry = symTab->tabAt(index);
        node.tabIdx = index;
        node.inferredType = entry.type;
        node.typeRef = entry.ref;
        node.level = entry.lev;

        if (entry.obj != semantic::ObjectKind::Procedure && entry.obj != semantic::ObjectKind::Function)
        {
            ScopeBuilder::report(node, "identifier '" + node.name + "' tidak dapat dipanggil");
        }

        for (auto& arg: node.args) visit(arg);
    }
}

void ScopeBuilder::visitIfNode(semantic::IfNode& node)
{
    inferExpression(node.condition.get());
    visit(node.thenStmt);
    visit(node.elseStmt);
}
void ScopeBuilder::visitCaseBranchNode(semantic::CaseBranchNode& node)
{
    for (auto& label: node.labels) inferExpression(label.get());
    visit(node.statement);
}
void ScopeBuilder::visitCaseNode(semantic::CaseNode& node)
{
    inferExpression(node.selector.get());
    for (auto& branch: node.branches) visit(branch.get());
}

void ScopeBuilder::visitWhileNode(semantic::WhileNode& node)
{
    inferExpression(node.condition.get());
    visit(node.body);
}
void ScopeBuilder::visitRepeatNode(semantic::RepeatNode& node)
{
    for (auto& statement: node.body) visit(statement);
    inferExpression(node.condition.get());
}

void ScopeBuilder::visitForNode(semantic::ForNode& node)
{
    const int index = lookupIdentifier(node, node.controlVar);
    if (index != semantic::NO_INDEX)
    {
        const auto& entry = symTab->tabAt(index);
        node.tabIdx = index;
        node.inferredType = entry.type;
        node.typeRef = entry.ref;
        node.level = entry.lev;
    }

    inferExpression(node.startExpr.get());
    inferExpression(node.endExpr.get());
    visit(node.body);
}
void ScopeBuilder::visitCompoundNode(semantic::CompoundNode& node)
{
    for (auto& statement: node.statements) visit(statement);
}
void ScopeBuilder::visitBinOpNode(semantic::BinOpNode& node)
{
    TypeInfo lhs = inferExpression(node.lhs.get());
    TypeInfo rhs = inferExpression(node.rhs.get());

    switch (node.op)
    {
        case semantic::BinOpKind::Eq:
        case semantic::BinOpKind::Ne:
        case semantic::BinOpKind::Lt:
        case semantic::BinOpKind::Le:
        case semantic::BinOpKind::Gt:
        case semantic::BinOpKind::Ge:
            node.inferredType = semantic::TypeKind::Boolean;
            node.typeRef = semantic::NO_INDEX;
            return;
        case semantic::BinOpKind::Div:
            node.inferredType = semantic::TypeKind::Real;
            node.typeRef = semantic::NO_INDEX;
            return;
        case semantic::BinOpKind::And:
        case semantic::BinOpKind::Or:
            node.inferredType = semantic::TypeKind::Boolean;
            node.typeRef = semantic::NO_INDEX;
            return;
        default:
            if (lhs.type == semantic::TypeKind::Real || rhs.type == semantic::TypeKind::Real)
            {
                node.inferredType = semantic::TypeKind::Real;
            }
            else
            {
                node.inferredType = semantic::TypeKind::Integer;
            }
            node.typeRef = semantic::NO_INDEX;
            return;
    }
}
void ScopeBuilder::visitUnaryOpNode(semantic::UnaryOpNode& node)
{
    TypeInfo operand = inferExpression(node.operand.get());
    node.inferredType = (node.op == semantic::UnaryOpKind::Not) ? semantic::TypeKind::Boolean: operand.type;
    node.typeRef = operand.ref;
}
void ScopeBuilder::visitVarNode(semantic::VarNode& node)
{
    const int index = lookupIdentifier(node, node.name);
    if (index == semantic::NO_INDEX)
    {
        node.inferredType = semantic::TypeKind::Error;
        return;
    }
    const auto& entry = symTab->tabAt(index);
    node.tabIdx = index;
    node.inferredType = entry.type;
    node.typeRef = entry.ref;
    node.level = entry.lev;
}
void ScopeBuilder::visitIntLitNode(semantic::IntLitNode& node)
{
    node.inferredType = semantic::TypeKind::Integer;
    node.typeRef = semantic::NO_INDEX;
}
void ScopeBuilder::visitRealLitNode(semantic::RealLitNode& node)
{
    node.inferredType = semantic::TypeKind::Real;
    node.typeRef = semantic::NO_INDEX;
}
void ScopeBuilder::visitCharLitNode(semantic::CharLitNode& node)
{
    node.inferredType = semantic::TypeKind::Char;
    node.typeRef = semantic::NO_INDEX;
}
void ScopeBuilder::visitStringLitNode(semantic::StringLitNode& node)
{
    node.inferredType = semantic::TypeKind::String;
    node.typeRef = semantic::NO_INDEX;
}
void ScopeBuilder::visitBoolLitNode(semantic::BoolLitNode& node)
{
    node.inferredType = semantic::TypeKind::Boolean;
    node.typeRef = semantic::NO_INDEX;
}
void ScopeBuilder::visitArrayAccessNode(semantic::ArrayAccessNode& node)
{
    TypeInfo base = inferExpression(node.base.get());
    for (auto& index: node.indices)
    {
        inferExpression(index.get());
    }
    if (base.type != semantic::TypeKind::Array || base.ref == semantic::NO_INDEX)
    {
        report(node, "akses array diterapkan kepada ekspresi yang bukan array");
        return;
    }

    const auto& arrayEntry = symTab->atabAt(base.ref);
    node.inferredType = arrayEntry.etyp;
    node.typeRef = arrayEntry.eref;
    if (node.base) node.tabIdx = node.base->tabIdx;
}
void ScopeBuilder::visitFieldAccessNode(semantic::FieldAccessNode& node)
{
    TypeInfo base = inferExpression(node.base.get());
    if (node.base) node.tabIdx = node.base->tabIdx;
    if (base.type != semantic::TypeKind::Record || base.ref == semantic::NO_INDEX)
    {
        node.inferredType = semantic::TypeKind::Error;
        report(node, "akses field diterapkan pada ekspresi yang bukan record");
        return;
    }
    int current = symTab->btabAt(base.ref).last;
    while (current != semantic::NO_INDEX)
    {
        const auto& entry = symTab->tabAt(current);
        if (sameName(entry.identifier, node.field))
        {
            node.tabIdx = current;
            node.inferredType = entry.type;
            node.typeRef = entry.ref;
            node.level = entry.lev;
            return;
        }
        current = entry.link;
    }
    node.inferredType = semantic::TypeKind::Error;
    report(node, "field '" + node.field + "' tidak dideklarasikan di dalam record");
}

bool ScopeBuilder::sameName(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size()) return false;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        const auto a = static_cast<unsigned char>(lhs[i]);
        const auto b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b)) return false;
    }
    return true;
}

int ScopeBuilder::primitiveSize(semantic::TypeKind type)
{
    switch(type)
    {
        case semantic::TypeKind::Integer: return 1;
        case semantic::TypeKind::Real: return 1;
        case semantic::TypeKind::Char: return 1;
        case semantic::TypeKind::Boolean: return 1;
        case semantic::TypeKind::String: return 1;
        case semantic::TypeKind::Void: return 0;
        default: return 1;
    }
}