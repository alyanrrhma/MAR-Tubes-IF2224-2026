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

    // Kunjungi deklarasi global dan compound statement utama program
    visit(node.declaration.get());
    visit(node.statements.get());
}

void ScopeBuilder::visitConstDeclNode(semantic::ConstDeclNode& node)
{
    TypeInfo type = inferExpression(node.value.get());
    node.inferredType = type.type;
    node.typeRef = type.ref;
    int adr = 0;
    bool ok = false;
    adr = static_cast<int>(constIntValue(node.value.get(), ok));
    node.tabIdx = declareIdentifier(
        node,
        node.name,
        semantic::ObjectKind::Constant,
        type,
        ok ? adr : 0);
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

    const int outerIndex = symTab->lookup(param.name);
    if (outerIndex != semantic::NO_INDEX) {
        const auto& outer = symTab->tabAt(outerIndex);
        if (outer.lev < symTab->currentLevel() &&
            (outer.obj == semantic::ObjectKind::Constant ||
             outer.obj == semantic::ObjectKind::Variable)) {
            report(*owner, "parameter formal '" + param.name +
                           "' tidak boleh men-shadow identifier global '" +
                           outer.identifier + "'");
            return;
        }
    }

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

    // Daftarkan parameter; btab.lpar = indeks parameter terakhir
    for (auto& param: node.params) {visitFormalParam(param);}
    // Set lpar ke identifier terakhir yang merupakan parameter
    symTab->btabAt(blockIndex).lpar = symTab->btabAt(blockIndex).last;
    // psze = total ukuran parameter (tiap param = 1 unit)
    symTab->btabAt(blockIndex).psze = static_cast<int>(node.params.size());

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

    // Daftarkan parameter; btab.lpar = indeks parameter terakhir
    for (auto& param: node.params) {visitFormalParam(param);}
    symTab->btabAt(blockIndex).lpar = symTab->btabAt(blockIndex).last;
    symTab->btabAt(blockIndex).psze = static_cast<int>(node.params.size());

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

void ScopeBuilder::visitReturnNode(semantic::ReturnNode& node)
{
    TypeInfo value = inferExpression(node.value.get());
    node.inferredType = value.type;
    node.typeRef = value.ref;
    if (node.value) node.tabIdx = node.value->tabIdx;
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
    visit(node.body.get());
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
    // Simpan panjang string di typeRef agar type_checker bisa validasi
    // kompatibilitas string (spesifikasi §II.C: string kompatibel jika panjang sama)
    node.typeRef = static_cast<int>(node.value.size());
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
        TypeInfo indexType = resolveTypeExpr(range);
        int low = 0;
        int high = 0;
        semantic::TypeKind indexBaseType = indexType.type;

        if (range && range->getKind() == semantic::AstKind::Range) {
            auto* rangeNode = static_cast<semantic::RangeNode*>(range);
            bool lowOk = false;
            bool highOk = false;
            low = static_cast<int>(constIntValue(rangeNode->low.get(), lowOk));
            high = static_cast<int>(constIntValue(rangeNode->high.get(), highOk));

            if (!lowOk || !highOk) {
                report(*range, "batas array harus berupa konstanta ordinal");
            }

            if (indexType.type == semantic::TypeKind::Subrange &&
                indexType.ref != semantic::NO_INDEX) {
                indexBaseType = symTab->atabAt(indexType.ref).xtyp;
            }
        } else {
            switch (indexType.type) {
                case semantic::TypeKind::Char:
                    low = 0;
                    high = 255;
                    break;
                case semantic::TypeKind::Boolean:
                    low = 0;
                    high = 1;
                    break;
                case semantic::TypeKind::Subrange:
                    if (indexType.ref != semantic::NO_INDEX) {
                        const auto& entry = symTab->atabAt(indexType.ref);
                        low = entry.low;
                        high = entry.high;
                        indexBaseType = entry.xtyp;
                    }
                    break;
                case semantic::TypeKind::Integer:
                case semantic::TypeKind::Enumerated:
                    low = 0;
                    high = 0;
                    break;
                default:
                    if (range) report(*range, "tipe indeks array harus ordinal dan bukan Real/String/Record/Array");
                    break;
            }
        }

        const int arrayRef = symTab->addArrayType(
            low,
            high,
            indexBaseType,
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
    const long long lowVal  = constIntValue(node.low.get(),  lowOk);
    const long long highVal = constIntValue(node.high.get(), highOk);

    // Inferensikan tipe literal low & high
    TypeInfo lowType  = inferExpression(node.low.get());
    TypeInfo highType = inferExpression(node.high.get());

    // Aturan spesifikasi: subrange tidak boleh bertipe Real
    if (lowType.type  == semantic::TypeKind::Real ||
        highType.type == semantic::TypeKind::Real) {
        report(node, "subrange tidak boleh bertipe Real");
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    // Batas harus berupa konstanta integer (atau char/boolean ordinal)
    if (!lowOk || !highOk) {
        report(node, "batas range harus berupa konstanta integer");
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    semantic::TypeKind baseType = semantic::TypeKind::Unknown;
    const auto normalizeOrdinal = [](semantic::TypeKind t) {
        return t == semantic::TypeKind::Subrange ? semantic::TypeKind::Integer : t;
    };
    const auto lowBase = normalizeOrdinal(lowType.type);
    const auto highBase = normalizeOrdinal(highType.type);

    if (lowBase == highBase &&
        (lowBase == semantic::TypeKind::Integer ||
         lowBase == semantic::TypeKind::Char ||
         lowBase == semantic::TypeKind::Boolean ||
         lowBase == semantic::TypeKind::Enumerated)) {
        baseType = lowBase;
    } else {
        report(node, "batas range harus memiliki tipe ordinal yang sama");
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    // Aturan spesifikasi: lower bound tidak boleh lebih besar dari upper bound
    if (lowVal > highVal) {
        report(node, "lower bound (" + std::to_string(lowVal) +
               ") tidak boleh lebih besar dari upper bound (" +
               std::to_string(highVal) + ")");
        node.inferredType = semantic::TypeKind::Error;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
    }

    // Simpan bounds di atab agar type_checker bisa cek assignment value
    // xtyp/etyp menyimpan tipe dasar subrange (integer/char/boolean/enumerated)
    const int size = static_cast<int>(highVal - lowVal + 1);
    const int atabIdx = symTab->addArrayType(
        static_cast<int>(lowVal),
        static_cast<int>(highVal),
        baseType,
        baseType,
        semantic::NO_INDEX,
        1  // elsz
    );
    // Perbaiki size di atab (addArrayType belum set size dengan tepat untuk subrange)
    symTab->atabAt(atabIdx).size = size;

    node.inferredType = semantic::TypeKind::Subrange;
    node.typeRef = atabIdx;
    return {semantic::TypeKind::Subrange, atabIdx, 1};
}

ScopeBuilder::TypeInfo ScopeBuilder::resolveEnumeratedType(semantic::EnumeratedTypeNode& node)
{
    std::vector<int> constantIndexes;
    bool hasError = false;
    semantic::TypeKind commonType = semantic::TypeKind::Unknown;

    const auto baseOrdinalType = [&](const semantic::TabEntry& entry) {
        if (entry.type == semantic::TypeKind::Subrange && entry.ref != semantic::NO_INDEX) {
            return symTab->atabAt(entry.ref).xtyp;
        }
        return entry.type;
    };

    for (const auto& identifier : node.identifiers) {
        const int index = symTab->lookup(identifier);
        if (index == semantic::NO_INDEX) {
            report(node, "identifier '" + identifier + "' belum dideklarasikan");
            hasError = true;
            continue;
        }

        const auto& entry = symTab->tabAt(index);
        if (entry.obj != semantic::ObjectKind::Constant) {
            report(node, "identifier '" + identifier + "' pada enumerated harus berupa konstanta");
            hasError = true;
            continue;
        }

        const semantic::TypeKind baseType = baseOrdinalType(entry);
        if (baseType != semantic::TypeKind::Integer &&
            baseType != semantic::TypeKind::Char &&
            baseType != semantic::TypeKind::Boolean &&
            baseType != semantic::TypeKind::Enumerated) {
            report(node, "identifier '" + identifier + "' pada enumerated harus bertipe ordinal");
            hasError = true;
            continue;
        }

        if (commonType == semantic::TypeKind::Unknown) {
            commonType = baseType;
        } else if (commonType != baseType) {
            report(node, "semua identifier pada enumerated harus memiliki tipe yang sama");
            hasError = true;
            continue;
        }

        constantIndexes.push_back(index);
    }

    if (hasError) {
        node.inferredType = semantic::TypeKind::Error;
        node.typeRef = semantic::NO_INDEX;
        return {semantic::TypeKind::Error, semantic::NO_INDEX, 1};
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
    case semantic::AstKind::CharLit: {
        const auto& lit = static_cast<semantic::CharLitNode&>(*node);
        ok = true;
        return static_cast<unsigned char>(lit.value);
    }
    case semantic::AstKind::BoolLit: {
        const auto& lit = static_cast<semantic::BoolLitNode&>(*node);
        ok = true;
        return lit.value ? 1 : 0;
    }
    case semantic::AstKind::Var: {
        auto& var = static_cast<semantic::VarNode&>(*node);
        if (var.tabIdx == semantic::NO_INDEX) {
            visitVarNode(var);
        }
        if (var.tabIdx == semantic::NO_INDEX) {
            return 0;
        }
        const auto& entry = symTab->tabAt(var.tabIdx);
        if (entry.obj != semantic::ObjectKind::Constant) {
            return 0;
        }
        if (entry.type == semantic::TypeKind::Integer ||
            entry.type == semantic::TypeKind::Char ||
            entry.type == semantic::TypeKind::Boolean ||
            entry.type == semantic::TypeKind::Enumerated ||
            entry.type == semantic::TypeKind::Subrange) {
            ok = true;
            return entry.adr;
        }
        return 0;
    }
    default:
        return 0;
    }
}

int ScopeBuilder::declareIdentifier(
    semantic::AstNode& owner, const std::string& name,
    semantic::ObjectKind obj,
    TypeInfo type,
    int adr,
    bool nrm
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

void ScopeBuilder::visit(semantic::AstNode* node)
{
    if (!node) return;
    node->level = symTab->currentLevel();

    switch (node->getKind())
    {
        case semantic::AstKind::Program: visitProgramNode(static_cast<semantic::ProgramNode&>(*node)); break;
        case semantic::AstKind::DeclPart: visitDeclarationNode(static_cast<semantic::DeclarationNode&>(*node)); break;
        case semantic::AstKind::Block: visitBlockNode(static_cast<semantic::BlockNode&>(*node), false); break;
        case semantic::AstKind::ConstDecl: visitConstDeclNode(static_cast<semantic::ConstDeclNode&>(*node)); break;
        case semantic::AstKind::TypeDecl: visitTypeDeclNode(static_cast<semantic::TypeDeclNode&>(*node)); break;
        case semantic::AstKind::VarDecl: visitVarDeclNode(static_cast<semantic::VarDeclNode&>(*node)); break;
        case semantic::AstKind::ProcDecl: visitProcDeclNode(static_cast<semantic::ProcDeclNode&>(*node)); break;
        case semantic::AstKind::FuncDecl: visitFuncDeclNode(static_cast<semantic::FuncDeclNode&>(*node)); break;
        case semantic::AstKind::SimpleType: visitSimpleTypeNode(static_cast<semantic::SimpleTypeNode&>(*node)); break;
        case semantic::AstKind::ArrayType: visitArrayTypeNode(static_cast<semantic::ArrayTypeNode&>(*node)); break;
        case semantic::AstKind::RecordType: visitRecordTypeNode(static_cast<semantic::RecordTypeNode&>(*node)); break;
        case semantic::AstKind::Range: visitRangeNode(static_cast<semantic::RangeNode&>(*node)); break;
        case semantic::AstKind::EnumeratedType: visitEnumeratedTypeNode(static_cast<semantic::EnumeratedTypeNode&>(*node)); break;
        case semantic::AstKind::Assign: visitAssignNode(static_cast<semantic::AssignNode&>(*node)); break;
        case semantic::AstKind::Return: visitReturnNode(static_cast<semantic::ReturnNode&>(*node)); break;
        case semantic::AstKind::ProcCall: visitProcCallNode(static_cast<semantic::ProcCallNode&>(*node)); break;
        case semantic::AstKind::If: visitIfNode(static_cast<semantic::IfNode&>(*node)); break;
        case semantic::AstKind::Case: visitCaseNode(static_cast<semantic::CaseNode&>(*node)); break;
        case semantic::AstKind::CaseBranch: visitCaseBranchNode(static_cast<semantic::CaseBranchNode&>(*node)); break;
        case semantic::AstKind::While: visitWhileNode(static_cast<semantic::WhileNode&>(*node)); break;
        case semantic::AstKind::Repeat: visitRepeatNode(static_cast<semantic::RepeatNode&>(*node)); break;
        case semantic::AstKind::For: visitForNode(static_cast<semantic::ForNode&>(*node)); break;
        case semantic::AstKind::Compound: visitCompoundNode(static_cast<semantic::CompoundNode&>(*node)); break;
        case semantic::AstKind::BinOp: visitBinOpNode(static_cast<semantic::BinOpNode&>(*node)); break;
        case semantic::AstKind::UnaryOp: visitUnaryOpNode(static_cast<semantic::UnaryOpNode&>(*node)); break;
        case semantic::AstKind::Var: visitVarNode(static_cast<semantic::VarNode&>(*node)); break;
        case semantic::AstKind::IntLit: visitIntLitNode(static_cast<semantic::IntLitNode&>(*node)); break;
        case semantic::AstKind::RealLit: visitRealLitNode(static_cast<semantic::RealLitNode&>(*node)); break;
        case semantic::AstKind::CharLit: visitCharLitNode(static_cast<semantic::CharLitNode&>(*node)); break;
        case semantic::AstKind::StringLit: visitStringLitNode(static_cast<semantic::StringLitNode&>(*node)); break;
        case semantic::AstKind::BoolLit: visitBoolLitNode(static_cast<semantic::BoolLitNode&>(*node)); break;
        case semantic::AstKind::ArrayAccess: visitArrayAccessNode(static_cast<semantic::ArrayAccessNode&>(*node)); break;
        case semantic::AstKind::FieldAccess: visitFieldAccessNode(static_cast<semantic::FieldAccessNode&>(*node)); break;
    }
}
