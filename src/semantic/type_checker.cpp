#include "type_checker.hpp"
#include "ast_nodes.hpp"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <iostream>
#include <unordered_map>

TypeChecker::TypeChecker() = default;

void TypeChecker::check(semantic::AstNode* root, semantic::SymbolTable& symTab)
{
    sym_ = &symTab;
    errors_.clear();
    loopDepth_ = 0;
    assignmentTargetDepth_ = 0;
    suppressGlobalInitCheck_ = false;
    initialized_.clear();
    seedInitiallyInitialized();
    visit(root);
}

void TypeChecker::printErrors(std::ostream& out) const
{
    for (const auto& e : errors_) {
        out << e << '\n';
    }
}

void TypeChecker::visit(semantic::AstNode* node)
{
    if (!node) return;

    switch (node->getKind()) {
        case semantic::AstKind::Program:
            visitProgram(static_cast<semantic::ProgramNode&>(*node)); break;
        case semantic::AstKind::DeclPart:
            visitDeclarations(static_cast<semantic::DeclarationNode&>(*node)); break;
        case semantic::AstKind::Block:
            visitBlock(static_cast<semantic::BlockNode&>(*node)); break;
        case semantic::AstKind::ConstDecl:
            visitConstDecl(static_cast<semantic::ConstDeclNode&>(*node)); break;
        case semantic::AstKind::TypeDecl:
            visitTypeDecl(static_cast<semantic::TypeDeclNode&>(*node)); break;
        case semantic::AstKind::VarDecl:
            visitVarDecl(static_cast<semantic::VarDeclNode&>(*node)); break;
        case semantic::AstKind::ProcDecl:
            visitProcDecl(static_cast<semantic::ProcDeclNode&>(*node)); break;
        case semantic::AstKind::FuncDecl:
            visitFuncDecl(static_cast<semantic::FuncDeclNode&>(*node)); break;
        case semantic::AstKind::Assign:
            visitAssign(static_cast<semantic::AssignNode&>(*node)); break;
        case semantic::AstKind::Return:
            visitReturn(static_cast<semantic::ReturnNode&>(*node)); break;
        case semantic::AstKind::ProcCall:
            visitProcCall(static_cast<semantic::ProcCallNode&>(*node)); break;
        case semantic::AstKind::If:
            visitIf(static_cast<semantic::IfNode&>(*node)); break;
        case semantic::AstKind::Case:
            visitCase(static_cast<semantic::CaseNode&>(*node)); break;
        case semantic::AstKind::CaseBranch:
            visitCaseBranch(static_cast<semantic::CaseBranchNode&>(*node)); break;
        case semantic::AstKind::While:
            visitWhile(static_cast<semantic::WhileNode&>(*node)); break;
        case semantic::AstKind::Repeat:
            visitRepeat(static_cast<semantic::RepeatNode&>(*node)); break;
        case semantic::AstKind::For:
            visitFor(static_cast<semantic::ForNode&>(*node)); break;
        case semantic::AstKind::Compound:
            visitCompound(static_cast<semantic::CompoundNode&>(*node)); break;
        case semantic::AstKind::BinOp:
            visitBinOp(static_cast<semantic::BinOpNode&>(*node)); break;
        case semantic::AstKind::UnaryOp:
            visitUnaryOp(static_cast<semantic::UnaryOpNode&>(*node)); break;
        case semantic::AstKind::Var:
            visitVar(static_cast<semantic::VarNode&>(*node)); break;
        case semantic::AstKind::ArrayAccess:
            visitArrayAccess(static_cast<semantic::ArrayAccessNode&>(*node)); break;
        case semantic::AstKind::FieldAccess:
            visitFieldAccess(static_cast<semantic::FieldAccessNode&>(*node)); break;
        case semantic::AstKind::IntLit:
            visitIntLit(static_cast<semantic::IntLitNode&>(*node)); break;
        case semantic::AstKind::RealLit:
            visitRealLit(static_cast<semantic::RealLitNode&>(*node)); break;
        case semantic::AstKind::CharLit:
            visitCharLit(static_cast<semantic::CharLitNode&>(*node)); break;
        case semantic::AstKind::StringLit:
            visitStringLit(static_cast<semantic::StringLitNode&>(*node)); break;
        case semantic::AstKind::BoolLit:
            visitBoolLit(static_cast<semantic::BoolLitNode&>(*node)); break;
        default:
            break;
    }
}

void TypeChecker::visitProgram(semantic::ProgramNode& n)
{
    visit(n.declaration.get());
    visit(n.statements.get());
}

void TypeChecker::visitDeclarations(semantic::DeclarationNode& n)
{
    for (auto& decl : n.declarations) {
        visit(decl.get());
    }
}

void TypeChecker::visitBlock(semantic::BlockNode& n)
{
    visit(n.declaration.get());
    visit(n.statements.get());
}

void TypeChecker::visitConstDecl(semantic::ConstDeclNode& n)
{
    visit(n.value.get());
}

void TypeChecker::visitTypeDecl(semantic::TypeDeclNode& n)
{
    if (!n.typeExpr ||
        n.typeExpr->getKind() != semantic::AstKind::EnumeratedType) {
        return;
    }

    const auto& enumerated = static_cast<const semantic::EnumeratedTypeNode&>(*n.typeExpr);
    for (const auto& identifier : enumerated.identifiers) {
        const int index = sym_->lookup(identifier);
        if (index == semantic::NO_INDEX) continue;

        const auto& entry = sym_->tabAt(index);
        if (entry.obj != semantic::ObjectKind::Constant) continue;

        if (entry.type != semantic::TypeKind::Integer) {
            report(n, "type mismatch dalam deklarasi tipe enumerated, expected integer found " +
                      std::string(semantic::toString(entry.type)) + " (" + identifier + ")");
            return;
        }
    }
}

void TypeChecker::visitVarDecl(semantic::VarDeclNode& /*n*/){}

void TypeChecker::visitProcDecl(semantic::ProcDeclNode& n)
{
    if (n.block) {
        const bool previous = suppressGlobalInitCheck_;
        suppressGlobalInitCheck_ = true;
        visit(n.block->declaration.get());
        visit(n.block->statements.get());
        suppressGlobalInitCheck_ = previous;
    }
}

void TypeChecker::visitFuncDecl(semantic::FuncDeclNode& n)
{
    if (n.block) {
        const bool previousInit = suppressGlobalInitCheck_;
        const int previousFunction = currentFunctionIdx_;
        suppressGlobalInitCheck_ = true;
        currentFunctionIdx_ = n.tabIdx;
        visit(n.block->declaration.get());
        visit(n.block->statements.get());
        currentFunctionIdx_ = previousFunction;
        suppressGlobalInitCheck_ = previousInit;

        if (!statementMustReturn(n.block->statements.get(), n.tabIdx)) {
            report(n, "fungsi '" + n.name + "' tidak memiliki return statement");
        }
    }
}

void TypeChecker::visitAssign(semantic::AssignNode& n)
{
    ++assignmentTargetDepth_;
    visit(n.target.get());
    --assignmentTargetDepth_;
    visit(n.value.get());

    if (!n.target || !n.value) return;

    const semantic::TypeKind lhsType = n.target->inferredType;
    const semantic::TypeKind rhsType = n.value->inferredType;

    if (lhsType == semantic::TypeKind::Error ||
        rhsType == semantic::TypeKind::Error ||
        lhsType == semantic::TypeKind::Unknown ||
        rhsType == semantic::TypeKind::Unknown) {
        return;
    }

    const semantic::AstKind targetKind = n.target->getKind();
    if (targetKind != semantic::AstKind::Var &&
        targetKind != semantic::AstKind::ArrayAccess &&
        targetKind != semantic::AstKind::FieldAccess) {
        report(n, "sisi kiri assignment harus berupa variabel atau elemen array/record");
        n.inferredType = semantic::TypeKind::Error;
        return;
    }

    const int baseIndex = assignmentBaseIndex(n.target.get());
    if (baseIndex != semantic::NO_INDEX) {
        const auto& entry = sym_->tabAt(baseIndex);
        if (entry.obj == semantic::ObjectKind::Constant) {
            report(n, "tidak boleh melakukan assignment terhadap konstanta '" + entry.identifier + "'");
            n.inferredType = semantic::TypeKind::Error;
            return;
        }
        if (entry.obj == semantic::ObjectKind::Function &&
            !isFunctionReturnAssign(n, currentFunctionIdx_)) {
            report(n, "assignment ke nama fungsi '" + entry.identifier +
                      "' hanya boleh dilakukan di dalam fungsi tersebut sebagai nilai return");
            n.inferredType = semantic::TypeKind::Error;
            return;
        }
        if (!isAssignableObject(entry.obj)) {
            report(n, "sisi kiri assignment harus berupa variabel, parameter, atau elemen array/record");
            n.inferredType = semantic::TypeKind::Error;
            return;
        }
    }

    if (!assignmentCompatible(lhsType, n.target->typeRef,
                              rhsType, n.value->typeRef)) {
        std::ostringstream msg;
        msg << "type mismatch dalam assignment: tidak dapat mengassign '"
            << typeName(rhsType) << "' ke '" << typeName(lhsType) << "'";
        report(n, msg.str());
        n.inferredType = semantic::TypeKind::Error;
        return;
    }

    if (lhsType == semantic::TypeKind::Subrange) {
        checkSubrangeBounds(n, n.target->typeRef, n.value.get());
    }

    markInitialized(n.target.get());
    n.inferredType = semantic::TypeKind::Void;
}

void TypeChecker::visitReturn(semantic::ReturnNode& n)
{
    visit(n.value.get());

    if (currentFunctionIdx_ == semantic::NO_INDEX || !n.value) {
        return;
    }

    const auto& functionEntry = sym_->tabAt(currentFunctionIdx_);
    if (functionEntry.obj != semantic::ObjectKind::Function) {
        return;
    }

    if (n.value->inferredType == semantic::TypeKind::Error ||
        n.value->inferredType == semantic::TypeKind::Unknown) {
        return;
    }

    if (!assignmentCompatible(functionEntry.type, functionEntry.ref,
                              n.value->inferredType, n.value->typeRef)) {
        std::ostringstream msg;
        msg << "type mismatch pada return: diharapkan '"
            << typeName(functionEntry.type) << "', mendapat '"
            << typeName(n.value->inferredType) << "'";
        report(n, msg.str());
    }
}

void TypeChecker::visitProcCall(semantic::ProcCallNode& n)
{
    for (auto& arg : n.args) {
        visit(arg.get());
    }

    if (n.tabIdx == semantic::NO_INDEX) return;

    const auto& callee = sym_->tabAt(n.tabIdx);
    if ((callee.obj != semantic::ObjectKind::Procedure &&
         callee.obj != semantic::ObjectKind::Function) ||
        callee.ref == semantic::NO_INDEX) {
        return;
    }

    std::vector<int> params;
    int current = sym_->btabAt(callee.ref).lpar;
    while (current != semantic::NO_INDEX) {
        const auto& entry = sym_->tabAt(current);
        if (entry.obj != semantic::ObjectKind::Parameter) break;
        params.push_back(current);
        current = entry.link;
    }
    std::reverse(params.begin(), params.end());

    if (params.size() != n.args.size()) {
        std::ostringstream msg;
        msg << "jumlah argumen untuk '" << n.name << "' tidak sesuai: diharapkan "
            << params.size() << ", mendapat " << n.args.size();
        report(n, msg.str());
        return;
    }

    for (std::size_t i = 0; i < params.size(); ++i) {
        const auto& param = sym_->tabAt(params[i]);
        const auto* arg = n.args[i].get();
        if (!arg) continue;

        const auto argType = arg->inferredType;
        if (argType == semantic::TypeKind::Unknown ||
            argType == semantic::TypeKind::Error) {
            continue;
        }

        if (!assignmentCompatible(param.type, param.ref, argType, arg->typeRef)) {
            std::ostringstream msg;
            msg << "argumen ke-" << (i + 1) << " untuk '" << n.name
                << "' tidak kompatibel: diharapkan '" << typeName(param.type)
                << "', mendapat '" << typeName(argType) << "'";
            report(*arg, msg.str());
        }
    }
}

void TypeChecker::visitIf(semantic::IfNode& n)
{
    visit(n.condition.get());
    if (n.condition) {
        const semantic::TypeKind ct = n.condition->inferredType;
        if (ct != semantic::TypeKind::Boolean &&
            ct != semantic::TypeKind::Error &&
            ct != semantic::TypeKind::Unknown) {
            reportTypeMismatch(*n.condition, "kondisi if", semantic::TypeKind::Boolean, ct);
        }
    }

    visit(n.thenStmt.get());
    visit(n.elseStmt.get());
}

void TypeChecker::visitCase(semantic::CaseNode& n)
{
    visit(n.selector.get());
    if (n.selector) {
        const semantic::TypeKind st = n.selector->inferredType;
        std::unordered_map<long long, const semantic::AstNode*> seenLabels;
        if (st != semantic::TypeKind::Integer &&
            st != semantic::TypeKind::Char &&
            st != semantic::TypeKind::Boolean &&
            st != semantic::TypeKind::Subrange &&
            st != semantic::TypeKind::Enumerated &&
            st != semantic::TypeKind::Error &&
            st != semantic::TypeKind::Unknown) {
            std::ostringstream msg;
            msg << "selector dalam case harus bertipe ordinal, bukan '"
                << typeName(st) << "'";
            report(*n.selector, msg.str());
        }

        for (auto& branch : n.branches) {
            if (!branch) continue;
            for (auto& label : branch->labels) {
                visit(label.get());
                if (!label) continue;

                const auto lt = label->inferredType;
                if (lt == semantic::TypeKind::Unknown ||
                    lt == semantic::TypeKind::Error) {
                    continue;
                }

                if (!compatibleWithRef(st, n.selector->typeRef, lt, label->typeRef)) {
                    std::ostringstream msg;
                    msg << "label case bertipe '" << typeName(lt)
                        << "' tidak kompatibel dengan selector bertipe '"
                        << typeName(st) << "'";
                    report(*label, msg.str());
                    continue;
                }

                long long labelValue = 0;
                if (constantValue(label.get(), labelValue)) {
                    if (seenLabels.find(labelValue) != seenLabels.end()) {
                        std::ostringstream msg;
                        msg << "duplicate case (" << labelValue << ")";
                        report(*label, msg.str());
                    } else {
                        seenLabels.emplace(labelValue, label.get());
                    }
                }
            }
            visit(branch->statement.get());
        }
        return;
    }
    for (auto& branch : n.branches) visit(branch.get());
}

void TypeChecker::visitCaseBranch(semantic::CaseBranchNode& n)
{
    for (auto& label : n.labels) {
        visit(label.get());
    }
    visit(n.statement.get());
}

void TypeChecker::visitWhile(semantic::WhileNode& n)
{
    visit(n.condition.get());
    if (n.condition) {
        const semantic::TypeKind ct = n.condition->inferredType;
        if (ct != semantic::TypeKind::Boolean &&
            ct != semantic::TypeKind::Error &&
            ct != semantic::TypeKind::Unknown) {
            reportTypeMismatch(*n.condition, "kondisi while", semantic::TypeKind::Boolean, ct);
        }
    }

    ++loopDepth_;
    visit(n.body.get());
    --loopDepth_;
}

void TypeChecker::visitRepeat(semantic::RepeatNode& n)
{
    ++loopDepth_;
    visit(n.body.get());
    --loopDepth_;

    visit(n.condition.get());
    if (n.condition) {
        const semantic::TypeKind ct = n.condition->inferredType;
        if (ct != semantic::TypeKind::Boolean &&
            ct != semantic::TypeKind::Error &&
            ct != semantic::TypeKind::Unknown) {
            reportTypeMismatch(*n.condition, "kondisi until (repeat)", semantic::TypeKind::Boolean, ct);
        }
    }
}

void TypeChecker::visitFor(semantic::ForNode& n)
{
    bool validControlVar = false;
    if (n.tabIdx == semantic::NO_INDEX) {
        report(n, "variabel kontrol for '" + n.controlVar + "' belum dideklarasikan");
    } else {
        const auto& entry = sym_->tabAt(n.tabIdx);
        if (entry.obj != semantic::ObjectKind::Variable &&
            entry.obj != semantic::ObjectKind::Parameter)
        {
            report(n, "variabel kontrol for '" + n.controlVar + "' harus berupa variabel");
        }
        else {
            validControlVar = true;
        }

        const semantic::TypeKind ct = entry.type;
        if (ct != semantic::TypeKind::Integer &&
            ct != semantic::TypeKind::Subrange &&
            ct != semantic::TypeKind::Error &&
            ct != semantic::TypeKind::Unknown) {
            std::ostringstream msg;
            msg << "variabel kontrol for '" << n.controlVar
                << "' harus bertipe integer, bukan '" << typeName(ct) << "'";
            report(n, msg.str());
        }
    }

    visit(n.startExpr.get());
    if (n.startExpr) {
        const semantic::TypeKind st = n.startExpr->inferredType;
        if (st != semantic::TypeKind::Integer &&
            st != semantic::TypeKind::Subrange &&
            st != semantic::TypeKind::Error &&
            st != semantic::TypeKind::Unknown) {
            reportTypeMismatch(*n.startExpr, "nilai awal for", semantic::TypeKind::Integer, st);
        }
    }

    visit(n.endExpr.get());
    if (n.endExpr) {
        const semantic::TypeKind et = n.endExpr->inferredType;
        if (et != semantic::TypeKind::Integer &&
            et != semantic::TypeKind::Subrange &&
            et != semantic::TypeKind::Error &&
            et != semantic::TypeKind::Unknown) {
            reportTypeMismatch(*n.endExpr, "nilai akhir for", semantic::TypeKind::Integer, et);
        }
    }

    if (validControlVar) initialized_.insert(n.tabIdx);

    ++loopDepth_;
    visit(n.body.get());
    --loopDepth_;
}

void TypeChecker::visitCompound(semantic::CompoundNode& n)
{
    for (auto& stmt : n.statements) {
        visit(stmt.get());
    }
}

void TypeChecker::visitBinOp(semantic::BinOpNode& n)
{
    visit(n.lhs.get());
    visit(n.rhs.get());

    if (!n.lhs || !n.rhs) return;

    const semantic::TypeKind lt = n.lhs->inferredType;
    const semantic::TypeKind rt = n.rhs->inferredType;

    if (lt == semantic::TypeKind::Error || rt == semantic::TypeKind::Error) {
        n.inferredType = semantic::TypeKind::Error;
        return;
    }
    if (lt == semantic::TypeKind::Unknown || rt == semantic::TypeKind::Unknown) {
        return; 
    }

    switch (n.op) {
        case semantic::BinOpKind::Add:
        case semantic::BinOpKind::Sub:
        case semantic::BinOpKind::Mul: {
            if (!isArithmetic(lt)) {
                std::ostringstream msg;
                msg << "operand kiri operator '" << toString(n.op)
                    << "' harus bertipe Integer atau Real, bukan '" << typeName(lt) << "'";
                report(*n.lhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            if (!isArithmetic(rt)) {
                std::ostringstream msg;
                msg << "operand kanan operator '" << toString(n.op)
                    << "' harus bertipe Integer atau Real, bukan '" << typeName(rt) << "'";
                report(*n.rhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = (lt == semantic::TypeKind::Real || rt == semantic::TypeKind::Real)
                             ? semantic::TypeKind::Real
                             : semantic::TypeKind::Integer;
            n.typeRef = semantic::NO_INDEX;
            break;
        }

        case semantic::BinOpKind::Div: {
            if (!isArithmetic(lt)) {
                std::ostringstream msg;
                msg << "operand kiri '/' harus bertipe Integer atau Real, bukan '"
                    << typeName(lt) << "'";
                report(*n.lhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            if (!isArithmetic(rt)) {
                std::ostringstream msg;
                msg << "operand kanan '/' harus bertipe Integer atau Real, bukan '"
                    << typeName(rt) << "'";
                report(*n.rhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = semantic::TypeKind::Real;
            n.typeRef = semantic::NO_INDEX;
            break;
        }

        case semantic::BinOpKind::IntDiv:
        case semantic::BinOpKind::Mod: {
            if (lt != semantic::TypeKind::Integer && lt != semantic::TypeKind::Subrange) {
                std::ostringstream msg;
                msg << "operand kiri '" << toString(n.op)
                    << "' harus bertipe Integer, bukan '" << typeName(lt) << "'";
                report(*n.lhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            if (rt != semantic::TypeKind::Integer && rt != semantic::TypeKind::Subrange) {
                std::ostringstream msg;
                msg << "operand kanan '" << toString(n.op)
                    << "' harus bertipe Integer, bukan '" << typeName(rt) << "'";
                report(*n.rhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = semantic::TypeKind::Integer;
            n.typeRef = semantic::NO_INDEX;
            break;
        }

        case semantic::BinOpKind::And:
        case semantic::BinOpKind::Or: {
            if (lt != semantic::TypeKind::Boolean) {
                std::ostringstream msg;
                msg << "operand kiri '" << toString(n.op)
                    << "' harus bertipe Boolean, bukan '" << typeName(lt) << "'";
                report(*n.lhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            if (rt != semantic::TypeKind::Boolean) {
                std::ostringstream msg;
                msg << "operand kanan '" << toString(n.op)
                    << "' harus bertipe Boolean, bukan '" << typeName(rt) << "'";
                report(*n.rhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = semantic::TypeKind::Boolean;
            n.typeRef = semantic::NO_INDEX;
            break;
        }

        case semantic::BinOpKind::Eq:
        case semantic::BinOpKind::Ne:
        case semantic::BinOpKind::Lt:
        case semantic::BinOpKind::Le:
        case semantic::BinOpKind::Gt:
        case semantic::BinOpKind::Ge: {
            if (!isRelational(lt)) {
                std::ostringstream msg;
                msg << "operand kiri operator relasional '" << toString(n.op)
                    << "' harus bertipe Integer, Real, Char, atau String, bukan '"
                    << typeName(lt) << "'";
                report(*n.lhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            if (!isRelational(rt)) {
                std::ostringstream msg;
                msg << "operand kanan operator relasional '" << toString(n.op)
                    << "' harus bertipe Integer, Real, Char, atau String, bukan '"
                    << typeName(rt) << "'";
                report(*n.rhs, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            if (!compatibleWithRef(lt, n.lhs->typeRef, rt, n.rhs->typeRef)) {
                std::ostringstream msg;
                msg << "perbandingan '" << toString(n.op)
                    << "' antara tipe '" << typeName(lt)
                    << "' dan '" << typeName(rt) << "' tidak kompatibel";
                report(n, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = semantic::TypeKind::Boolean;
            n.typeRef = semantic::NO_INDEX;
            break;
        }
    }
}

void TypeChecker::visitUnaryOp(semantic::UnaryOpNode& n)
{
    visit(n.operand.get());
    if (!n.operand) return;

    const semantic::TypeKind ot = n.operand->inferredType;

    if (ot == semantic::TypeKind::Error) {
        n.inferredType = semantic::TypeKind::Error;
        return;
    }

    switch (n.op) {
        case semantic::UnaryOpKind::Plus:
        case semantic::UnaryOpKind::Minus:
            if (!isArithmetic(ot)) {
                std::ostringstream msg;
                msg << "operator unary '" << toString(n.op)
                    << "' hanya berlaku untuk Integer atau Real, bukan '"
                    << typeName(ot) << "'";
                report(n, msg.str());
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = ot;
            break;

        case semantic::UnaryOpKind::Not:
            if (ot != semantic::TypeKind::Boolean) {
                reportTypeMismatch(n, "operand 'not'", semantic::TypeKind::Boolean, ot);
                n.inferredType = semantic::TypeKind::Error;
                return;
            }
            n.inferredType = semantic::TypeKind::Boolean;
            break;
    }
    n.typeRef = n.operand->typeRef;
}

void TypeChecker::visitVar(semantic::VarNode& n)
{
    if (n.tabIdx != semantic::NO_INDEX && n.inferredType == semantic::TypeKind::Unknown) {
        const auto& entry = sym_->tabAt(n.tabIdx);
        n.inferredType = entry.type;
        n.typeRef = entry.ref;
        n.level = entry.lev;
    }

    if (assignmentTargetDepth_ > 0 || n.tabIdx == semantic::NO_INDEX) return;

    const auto& entry = sym_->tabAt(n.tabIdx);
    if (suppressGlobalInitCheck_ && entry.lev == 0) return;
    if (entry.obj == semantic::ObjectKind::Variable && !isInitialized(n.tabIdx)) {
        report(n, "variabel '" + entry.identifier + "' digunakan sebelum diinisialisasi");
    }
}

void TypeChecker::visitArrayAccess(semantic::ArrayAccessNode& n)
{
    visit(n.base.get());

    semantic::TypeKind currentType =
        n.base ? n.base->inferredType : semantic::TypeKind::Unknown;
    int currentRef = n.base ? n.base->typeRef : semantic::NO_INDEX;

    for (auto& idx : n.indices) {
        visit(idx.get());
        if (!idx) continue;

        if (currentType != semantic::TypeKind::Array || currentRef == semantic::NO_INDEX) {
            report(n, "jumlah indeks melebihi dimensi array");
            n.inferredType = semantic::TypeKind::Error;
            return;
        }

        const auto& ent = sym_->atabAt(currentRef);
        const semantic::TypeKind it = idx->inferredType;
        const semantic::TypeKind expectedIndexType = ent.xtyp;
        if (it != semantic::TypeKind::Error &&
            it != semantic::TypeKind::Unknown &&
            expectedIndexType != semantic::TypeKind::Unknown &&
            !compatibleWithRef(expectedIndexType, semantic::NO_INDEX, it, idx->typeRef)) {
            reportTypeMismatch(*idx, "indeks array", expectedIndexType, it);
        }
        long long literalValue = 0;
        bool hasLiteral = false;

        switch (idx->getKind()) {
            case semantic::AstKind::IntLit:
                literalValue = static_cast<const semantic::IntLitNode&>(*idx).value;
                hasLiteral = true;
                break;
            case semantic::AstKind::CharLit:
                literalValue = static_cast<unsigned char>(
                    static_cast<const semantic::CharLitNode&>(*idx).value);
                hasLiteral = true;
                break;
            case semantic::AstKind::BoolLit:
                literalValue = static_cast<const semantic::BoolLitNode&>(*idx).value ? 1 : 0;
                hasLiteral = true;
                break;
            default:
                break;
        }

        if (hasLiteral && (literalValue < ent.low || literalValue > ent.high)) {
            std::ostringstream msg;
            msg << "indeks array " << literalValue << " di luar batas ["
                << ent.low << ".." << ent.high << "]";
            report(*idx, msg.str());
        }

        currentType = ent.etyp;
        currentRef = ent.eref;
    }

    n.inferredType = currentType;
    n.typeRef = currentRef;
}

void TypeChecker::visitFieldAccess(semantic::FieldAccessNode& n)
{
    visit(n.base.get());
    if (n.inferredType == semantic::TypeKind::Unknown && n.tabIdx != semantic::NO_INDEX) {
        const auto& entry = sym_->tabAt(n.tabIdx);
        n.inferredType = entry.type;
        n.typeRef = entry.ref;
    }
}

void TypeChecker::visitIntLit(semantic::IntLitNode& n)
{
    n.inferredType = semantic::TypeKind::Integer;
    n.typeRef = semantic::NO_INDEX;
}

void TypeChecker::visitRealLit(semantic::RealLitNode& n)
{
    n.inferredType = semantic::TypeKind::Real;
    n.typeRef = semantic::NO_INDEX;
}

void TypeChecker::visitCharLit(semantic::CharLitNode& n)
{
    n.inferredType = semantic::TypeKind::Char;
    n.typeRef = semantic::NO_INDEX;
}

void TypeChecker::visitStringLit(semantic::StringLitNode& n)
{
    n.inferredType = semantic::TypeKind::String;
    n.typeRef = static_cast<int>(n.value.size());
}

void TypeChecker::visitBoolLit(semantic::BoolLitNode& n)
{
    n.inferredType = semantic::TypeKind::Boolean;
    n.typeRef = semantic::NO_INDEX;
}

bool TypeChecker::compatible(semantic::TypeKind a, semantic::TypeKind b) const
{
    return compatibleWithRef(a, semantic::NO_INDEX, b, semantic::NO_INDEX);
}

bool TypeChecker::compatibleWithRef(semantic::TypeKind a, int aRef,
                                     semantic::TypeKind b, int bRef) const
{
    const auto subrangeBase = [&](int ref) -> semantic::TypeKind {
        if (ref == semantic::NO_INDEX) return semantic::TypeKind::Integer;
        return sym_->atabAt(ref).xtyp;
    };

    if (a == b) {
        if (a == semantic::TypeKind::Array) {
            return sameArrayTypeRef(aRef, bRef);
        }
        if (a == semantic::TypeKind::Record ||
            a == semantic::TypeKind::Enumerated) {
            return aRef != semantic::NO_INDEX && aRef == bRef;
        }
        if (a == semantic::TypeKind::String) {
            if (aRef == semantic::NO_INDEX || bRef == semantic::NO_INDEX) return true;
            return aRef == bRef;
        }
        return true;
    }

    if (a == semantic::TypeKind::Subrange && b == semantic::TypeKind::Subrange) {
        return subrangeBase(aRef) == subrangeBase(bRef);
    }

    if (a == semantic::TypeKind::Subrange) {
        return subrangeBase(aRef) == b;
    }

    if (b == semantic::TypeKind::Subrange) {
        return a == subrangeBase(bRef);
    }

    if ((a == semantic::TypeKind::Integer && b == semantic::TypeKind::Real) ||
        (a == semantic::TypeKind::Real    && b == semantic::TypeKind::Integer)) {
        return true;
    }

    return false;
}

bool TypeChecker::sameArrayTypeRef(int aRef, int bRef) const
{
    return aRef != semantic::NO_INDEX && aRef == bRef;
}

bool TypeChecker::sameArrayEntry(int aRef, int bRef, std::unordered_set<long long>& seen) const
{
    if (aRef == semantic::NO_INDEX || bRef == semantic::NO_INDEX) return false;
    if (aRef == bRef) return true;
    if (!sym_) return false;
    if (aRef < 0 || bRef < 0 ||
        aRef >= sym_->atabSize() || bRef >= sym_->atabSize()) {
        return false;
    }

    const long long key = (static_cast<long long>(aRef) << 32) ^
                          static_cast<unsigned int>(bRef);
    if (seen.count(key)) return true;
    seen.insert(key);

    const auto& lhs = sym_->atabAt(aRef);
    const auto& rhs = sym_->atabAt(bRef);

    if (lhs.low != rhs.low || lhs.high != rhs.high || lhs.xtyp != rhs.xtyp) {
        return false;
    }

    if (lhs.etyp != rhs.etyp) return false;

    if (lhs.etyp == semantic::TypeKind::Array) {
        return sameArrayEntry(lhs.eref, rhs.eref, seen);
    }

    if (lhs.etyp == semantic::TypeKind::Record ||
        lhs.etyp == semantic::TypeKind::Enumerated) {
        return lhs.eref != semantic::NO_INDEX && lhs.eref == rhs.eref;
    }

    if (lhs.etyp == semantic::TypeKind::String) {
        return lhs.eref == rhs.eref ||
               lhs.eref == semantic::NO_INDEX ||
               rhs.eref == semantic::NO_INDEX;
    }

    if (lhs.etyp == semantic::TypeKind::Subrange) {
        if (lhs.eref == rhs.eref) return true;
        if (lhs.eref == semantic::NO_INDEX || rhs.eref == semantic::NO_INDEX) return false;
        const auto& lrange = sym_->atabAt(lhs.eref);
        const auto& rrange = sym_->atabAt(rhs.eref);
        return lrange.low == rrange.low &&
               lrange.high == rrange.high &&
               lrange.xtyp == rrange.xtyp;
    }

    return true;
}

void TypeChecker::checkSubrangeBounds(const semantic::AstNode& node,
                                       int lhsRef, const semantic::AstNode* rhsNode) const
{
    if (lhsRef == semantic::NO_INDEX) return; 
    if (!rhsNode) return;

    long long val = 0;
    switch (rhsNode->getKind()) {
        case semantic::AstKind::IntLit:
            val = static_cast<const semantic::IntLitNode&>(*rhsNode).value;
            break;
        case semantic::AstKind::CharLit:
            val = static_cast<unsigned char>(
                static_cast<const semantic::CharLitNode&>(*rhsNode).value);
            break;
        case semantic::AstKind::BoolLit:
            val = static_cast<const semantic::BoolLitNode&>(*rhsNode).value ? 1 : 0;
            break;
        default:
            return;
    }

    const auto& entry = sym_->atabAt(lhsRef);
    if (val < entry.low || val > entry.high) {
        std::ostringstream msg;
        msg << "nilai " << val << " di luar batas subrange ["
            << entry.low << ".." << entry.high << "]";
        const_cast<TypeChecker*>(this)->report(node, msg.str());
    }
}

bool TypeChecker::isAssignableObject(semantic::ObjectKind obj) const
{
    return obj == semantic::ObjectKind::Variable ||
           obj == semantic::ObjectKind::Parameter ||
           obj == semantic::ObjectKind::Field ||
           obj == semantic::ObjectKind::Function;
}

int TypeChecker::assignmentBaseIndex(const semantic::AstNode* node) const
{
    if (!node) return semantic::NO_INDEX;

    switch (node->getKind()) {
        case semantic::AstKind::Var:
            return node->tabIdx;
        case semantic::AstKind::ArrayAccess: {
            const auto& access = static_cast<const semantic::ArrayAccessNode&>(*node);
            return assignmentBaseIndex(access.base.get());
        }
        case semantic::AstKind::FieldAccess: {
            const auto& access = static_cast<const semantic::FieldAccessNode&>(*node);
            return assignmentBaseIndex(access.base.get());
        }
        default:
            return node->tabIdx;
    }
}

bool TypeChecker::assignmentCompatible(semantic::TypeKind lhsType, int lhsRef,
                                        semantic::TypeKind rhsType, int rhsRef) const
{
    const auto subrangeBase = [&](int ref) -> semantic::TypeKind {
        if (ref == semantic::NO_INDEX) return semantic::TypeKind::Integer;
        return sym_->atabAt(ref).xtyp;
    };

    if (lhsType == rhsType) {
        if (lhsType == semantic::TypeKind::Array) {
            return sameArrayTypeRef(lhsRef, rhsRef);
        }

        if (lhsType == semantic::TypeKind::Record ||
            lhsType == semantic::TypeKind::Enumerated) {
            return lhsRef != semantic::NO_INDEX && lhsRef == rhsRef;
        }

        if (lhsType == semantic::TypeKind::String) {
            if (lhsRef == semantic::NO_INDEX || rhsRef == semantic::NO_INDEX) return true;
            return lhsRef == rhsRef;
        }

        return true;
    }

    if (lhsType == semantic::TypeKind::Real && rhsType == semantic::TypeKind::Integer) {
        return true;
    }

    if (lhsType == semantic::TypeKind::Enumerated && rhsType == semantic::TypeKind::Integer) {
        return true;
    }

    if (lhsType == semantic::TypeKind::Subrange && rhsType == semantic::TypeKind::Subrange) {
        return subrangeBase(lhsRef) == subrangeBase(rhsRef);
    }
    if (lhsType == semantic::TypeKind::Subrange) {
        return subrangeBase(lhsRef) == rhsType;
    }
    if (rhsType == semantic::TypeKind::Subrange) {
        return lhsType == subrangeBase(rhsRef);
    }

    if (lhsType == semantic::TypeKind::String && rhsType == semantic::TypeKind::String) {
        if (lhsRef == semantic::NO_INDEX || rhsRef == semantic::NO_INDEX) return true;
        return lhsRef == rhsRef;
    }

    return false;
}

bool TypeChecker::isArithmetic(semantic::TypeKind t) const
{
    return t == semantic::TypeKind::Integer ||
           t == semantic::TypeKind::Real ||
           t == semantic::TypeKind::Subrange; 
}

bool TypeChecker::isRelational(semantic::TypeKind t) const
{
    return t == semantic::TypeKind::Integer ||
           t == semantic::TypeKind::Real ||
           t == semantic::TypeKind::Char ||
           t == semantic::TypeKind::String ||
           t == semantic::TypeKind::Boolean ||
           t == semantic::TypeKind::Subrange;
}

std::string TypeChecker::typeName(semantic::TypeKind t)
{
    return toString(t); 
}

bool TypeChecker::isInitialized(int tabIdx) const
{
    return initialized_.find(tabIdx) != initialized_.end();
}

void TypeChecker::markInitialized(semantic::AstNode* node)
{
    if (!node) return;

    switch (node->getKind()) {
        case semantic::AstKind::Var: {
            auto& var = static_cast<semantic::VarNode&>(*node);
            if (var.tabIdx != semantic::NO_INDEX) initialized_.insert(var.tabIdx);
            break;
        }
        case semantic::AstKind::ArrayAccess: {
            auto& access = static_cast<semantic::ArrayAccessNode&>(*node);
            markInitialized(access.base.get());
            break;
        }
        case semantic::AstKind::FieldAccess: {
            auto& access = static_cast<semantic::FieldAccessNode&>(*node);
            markInitialized(access.base.get());
            break;
        }
        default:
            break;
    }
}

void TypeChecker::seedInitiallyInitialized()
{
    if (!sym_) return;

    for (int i = 0; i < sym_->tabSize(); ++i) {
        const auto& entry = sym_->tabAt(i);
        if (entry.obj == semantic::ObjectKind::Constant ||
            entry.obj == semantic::ObjectKind::Parameter) {
            initialized_.insert(i);
        }
    }
}

bool TypeChecker::constantValue(const semantic::AstNode* node, long long& value) const
{
    if (!node) return false;

    switch (node->getKind()) {
        case semantic::AstKind::IntLit:
            value = static_cast<const semantic::IntLitNode&>(*node).value;
            return true;
        case semantic::AstKind::CharLit:
            value = static_cast<unsigned char>(
                static_cast<const semantic::CharLitNode&>(*node).value);
            return true;
        case semantic::AstKind::BoolLit:
            value = static_cast<const semantic::BoolLitNode&>(*node).value ? 1 : 0;
            return true;
        case semantic::AstKind::UnaryOp: {
            const auto& unary = static_cast<const semantic::UnaryOpNode&>(*node);
            if (unary.op != semantic::UnaryOpKind::Plus &&
                unary.op != semantic::UnaryOpKind::Minus) {
                return false;
            }

            long long operandValue = 0;
            if (!constantValue(unary.operand.get(), operandValue)) return false;
            value = unary.op == semantic::UnaryOpKind::Minus ? -operandValue : operandValue;
            return true;
        }
        case semantic::AstKind::Var: {
            const auto& var = static_cast<const semantic::VarNode&>(*node);
            if (var.tabIdx == semantic::NO_INDEX) return false;

            const auto& entry = sym_->tabAt(var.tabIdx);
            if (entry.obj != semantic::ObjectKind::Constant) return false;

            value = entry.adr;
            return true;
        }
        default:
            return false;
    }
}

bool TypeChecker::isFunctionReturnAssign(const semantic::AssignNode& node, int functionIdx) const
{
    if (node.target == nullptr || node.target->getKind() != semantic::AstKind::Var) {
        return false;
    }

    const auto& target = static_cast<const semantic::VarNode&>(*node.target);
    return target.tabIdx == functionIdx;
}

bool TypeChecker::statementMustReturn(const semantic::AstNode* node, int functionIdx) const
{
    if (!node) return false;

    switch (node->getKind()) {
        case semantic::AstKind::Return:
            return true;
        case semantic::AstKind::Assign:
            return isFunctionReturnAssign(static_cast<const semantic::AssignNode&>(*node), functionIdx);
        case semantic::AstKind::Compound:
            return compoundMustReturn(static_cast<const semantic::CompoundNode&>(*node), functionIdx);
        case semantic::AstKind::If:
            return ifMustReturn(static_cast<const semantic::IfNode&>(*node), functionIdx);
        case semantic::AstKind::Case:
            return caseMustReturn(static_cast<const semantic::CaseNode&>(*node), functionIdx);
        case semantic::AstKind::Repeat:
            return repeatMustReturn(static_cast<const semantic::RepeatNode&>(*node), functionIdx);
        case semantic::AstKind::While:
        case semantic::AstKind::For:
            return false;
        default:
            return false;
    }
}

bool TypeChecker::compoundMustReturn(const semantic::CompoundNode& node, int functionIdx) const
{
    for (const auto& statement : node.statements) {
        if (statementMustReturn(statement.get(), functionIdx)) {
            return true;
        }
    }
    return false;
}

bool TypeChecker::ifMustReturn(const semantic::IfNode& node, int functionIdx) const
{
    if (!node.thenStmt || !node.elseStmt) {
        return false;
    }

    return statementMustReturn(node.thenStmt.get(), functionIdx) &&
           statementMustReturn(node.elseStmt.get(), functionIdx);
}

bool TypeChecker::caseMustReturn(const semantic::CaseNode& node, int functionIdx) const
{
    if (node.branches.empty()) {
        return false;
    }

    for (const auto& branch : node.branches) {
        if (!branch || !statementMustReturn(branch->statement.get(), functionIdx)) {
            return false;
        }
    }

    return true;
}

bool TypeChecker::repeatMustReturn(const semantic::RepeatNode& node, int functionIdx) const
{
    return statementMustReturn(node.body.get(), functionIdx);
}

void TypeChecker::report(const semantic::AstNode& node, const std::string& msg)
{
    std::ostringstream out;
    out << "[Type error]";
    if (node.line != semantic::NO_INDEX) {
        out << " line " << node.line;
        if (node.column != semantic::NO_INDEX) out << ", column " << node.column;
    }
    out << ": " << msg;
    errors_.push_back(out.str());
}

void TypeChecker::reportTypeMismatch(const semantic::AstNode& node,
                                      const std::string& context,
                                      semantic::TypeKind expected,
                                      semantic::TypeKind got)
{
    std::ostringstream msg;
    msg << "type mismatch pada " << context
        << ": diharapkan '" << typeName(expected)
        << "', tetapi mendapat '" << typeName(got) << "'";
    report(node, msg.str());
}
