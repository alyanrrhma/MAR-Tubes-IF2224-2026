#include "scope_builder.hpp"
#include <sstream>

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

ScopeBuilder::TypeInfo ScopeBuilder::resolveTypeExpr(semantic::AstNode* node)
{
    if (!node) return {semantic::TypeKind::Unknown, semantic::NO_INDEX, 1};

    switch (node->getKind())
    {
        case semantic::AstKind::SimpleType: return resolveSimpleType(static_cast<semantic::SimpleTypeNode&>(*node));
        case semantic::AstKind::ArrayType: return resolveArrayType(static_cast<semantic::ArrayTypeNode&>(*node));
        case semantic::AstKind::RecordType: return resolveRecordType(static_cast<semantic::RecordTypeNode&>(*node));
        case semantic::AstKind::Range: return resolveRangeType(static_cast<semantic::RangeNode&>(*node));
        case semantic::AstKind::EnumeratedType: return resolveEnumeratedType(static_cast<semantic::EnumeratedTypeNode&>(*node));
        default:
            visit(node);
            return {node->inferredType, node->typeRef, primitiveSize(node->inferredType)};
    }
}

ScopeBuilder::TypeInfo ScopeBuilder::resolveSimpleType(semantic::SimpleTypeNode& node)
{
    const int index = lookupIdentifier(node, node.name);
    if (index == semantic::NO_INDEX) {
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    const auto& entry = symTab->tabAt(index);
    if (entry.obj != semantic::ObjectKind::Type) {
        report(node, "identifier '" + node.name + "' bukan bentuk tipe");
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    node.tabIdx = index;
    node.inferredType = entry.type;
    node.typeRef = entry.ref;
    node.level = entry.lev;

    int size = primitiveSize(entry.type);
    if (entry.type == semantic::TypeKind::Array && entry.ref != semantic::NO_INDEX) {
        size = symTab->atabAt(entry.ref).size;
    }

    return {entry.type, entry.ref, size};
}

ScopeBuilder::TypeInfo ScopeBuilder::resolveArrayType(semantic::ArrayTypeNode& node)
{
    TypeInfo element = resolveTypeExpr(node.elementType.get());

    for (int i = static_cast<int>(node.indexTypes.size()) - 1; i >= 0; --i)
    {
        auto* range = node.indexTypes[static_cast<std::size_t>(i)].get();
        bool lowOk = false;
        bool highOk = false;
        const int low = static_cast<int>(constIntValue(static_cast<semantic::RangeNode*>(range)->low.get(), lowOk));
        const int high = static_cast<int>(constIntValue(static_cast<semantic::RangeNode*>(range)->high.get(), highOk));

        TypeInfo indexType = resolveTypeExpr(range);
        if (!lowOk || !highOk) {
            report(*range, "batas array harus berupa integer");
        }

        const int arrayRef = symTab->addArrayType(
            low,
            high,
            indexType.type,
            element.type,
            element.ref,
            element.size);
        element = {semantic::TypeKind::Array, arrayRef, symTab->atabAt(arrayRef).size};
    }

    node.inferredType = element.type;
    node.typeRef = element.ref;
    return element;
}

ScopeBuilder::TypeInfo ScopeBuilder::resolveRecordType(semantic::RecordTypeNode& node)
{
    const int blockIndex = symTab->pushBlock();
    node.level = symTab->currentLevel();
    int offset = 0;

    for (auto& section : node.fields) {
        TypeInfo fieldType = resolveTypeExpr(section.typeExpr.get());
        for (const auto& fieldName : section.names) {
            semantic::AstNode* owner = section.typeExpr.get();
            if (!owner) continue;
            if (symTab->lookupCurrentScope(fieldName) != semantic::NO_INDEX) {
                report(*owner, "redeklarasi dari field '" + fieldName + "'");
                continue;
            }
            symTab->insert(
                fieldName,
                semantic::ObjectKind::Field,
                fieldType.type,
                fieldType.ref,
                offset,
                true);
            offset += fieldType.size;
        }
    }

    symTab->popBlock();
    node.inferredType = semantic::TypeKind::Record;
    node.typeRef = blockIndex;
    return {semantic::TypeKind::Record, blockIndex, offset > 0 ? offset : 1};

}

ScopeBuilder::TypeInfo ScopeBuilder::resolveRangeType(semantic::RangeNode& node)
{
    bool lowOk = false;
    bool highOk = false;
    constIntValue(node.low.get(), lowOk);
    constIntValue(node.high.get(), highOk);

    inferExpression(node.low.get());
    inferExpression(node.high.get());

    if (!lowOk || !highOk) {
        report(node, "batas range harus berupa integer");
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    node.inferredType = semantic::TypeKind::Subrange;
    node.typeRef = semantic::NO_INDEX;
    return {semantic::TypeKind::Subrange, semantic::NO_INDEX, 1};
}

ScopeBuilder::TypeInfo ScopeBuilder::resolveEnumeratedType(semantic::EnumeratedTypeNode& node)
{
    int value = 0;
    for (const auto& identifier : node.identifiers) {
        if (symTab->lookupCurrentScope(identifier) != semantic::NO_INDEX) {
            report(node, "redeklarasi dari konstanta enumerasi '" + identifier + "'");
            continue;
        }
        symTab->insert(
            identifier,
            semantic::ObjectKind::Constant,
            semantic::TypeKind::Enumerated,
            semantic::NO_INDEX,
            value++,
            true);
    }

    node.inferredType = semantic::TypeKind::Enumerated;
    node.typeRef = semantic::NO_INDEX;
    return {semantic::TypeKind::Enumerated, semantic::NO_INDEX, 1};

}


ScopeBuilder::TypeInfo ScopeBuilder::inferExpression(semantic::AstNode* node)
{
    if (!node) return {semantic::TypeKind::Unknown, semantic::NO_INDEX, 1};
    visit(node);

    int size = primitiveSize(node->inferredType);
    if (node->inferredType == semantic::TypeKind::Array && node->typeRef != semantic::NO_INDEX) {
        size = symTab->atabAt(node->typeRef).size;
    }

    return {node->inferredType, node->typeRef, size};
}

long long ScopeBuilder::constIntValue(semantic::AstNode* node, bool& ok)
{
    ok = false;
    if (!node) return 0;

    switch (node->getKind()) {
    case semantic::AstKind::IntLit: {
        const auto& lit = static_cast<semantic::IntLitNode&>(*node);
        ok = true;
        return lit.value;
    }
    case semantic::AstKind::UnaryOp: {
        auto& unary = static_cast<semantic::UnaryOpNode&>(*node);
        if (unary.op != semantic::UnaryOpKind::Minus && unary.op != semantic::UnaryOpKind::Plus) {
            return 0;
        }
        bool childOk = false;
        const long long value = constIntValue(unary.operand.get(), childOk);
        ok = childOk;
        return unary.op == semantic::UnaryOpKind::Minus ? -value : value;
    }
    default:
        return 0;
    }
}

int ScopeBuilder::declareIdentifier(
    semantic::AstNode& owner, const std::string& name,
    semantic::ObjectKind obj,
    TypeInfo type,
    int adr = 0,
    bool nrm = true
)
{
    if (name.empty()) return semantic::NO_INDEX;

    if (symTab->lookupCurrentScope(name) != semantic::NO_INDEX) {
        report(owner, "redeklarasi identifier '" + name + "'");
        owner.inferredType = semantic::TypeKind::Error;
        return semantic::NO_INDEX;
    }

    const int index = symTab->insert(name, obj, type.type, type.ref, adr, nrm);
    owner.tabIdx = index;
    owner.inferredType = type.type;
    owner.typeRef = type.ref;
    owner.level = symTab->currentLevel();
    return index;
}

int ScopeBuilder::lookupIdentifier(semantic::AstNode& owner, const std::string& name)
{
    const int index = symTab->lookup(name);
    if (index == semantic::NO_INDEX) {
        report(owner, "identifier '" + name + "' belum dideklarasikan");
    }
    return index;
}

int ScopeBuilder::allocateAddress(int size)
{
    const int block = symTab->currentBlock();
    if (block == semantic::NO_INDEX) return 0;

    auto& btab = symTab->btabAt(block);
    const int adr = btab.vsze;
    btab.vsze += size > 0 ? size : 1;
    return adr;

}

void ScopeBuilder::report(const semantic::AstNode& node, const std::string& message)
{
    std::ostringstream out;
    out << "[Semantic error]";
    if (node.line != semantic::NO_INDEX) {
        out << " line " << node.line;
        if (node.column != semantic::NO_INDEX) out << ", column " << node.column;
    }
    out << ": " << message;
    errorMsg.push_back(out.str());
}