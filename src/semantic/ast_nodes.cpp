#include "ast_nodes.hpp"

#include <ostream>
#include <sstream>

namespace semantic {

const char* toString(AstKind kind) {
    switch (kind) {
    case AstKind::Program:        return "Program";
    case AstKind::DeclPart:       return "DeclPart";
    case AstKind::Block:          return "Block";
    case AstKind::ConstDecl:      return "ConstDecl";
    case AstKind::TypeDecl:       return "TypeDecl";
    case AstKind::VarDecl:        return "VarDecl";
    case AstKind::ProcDecl:       return "ProcDecl";
    case AstKind::FuncDecl:       return "FuncDecl";
    case AstKind::SimpleType:     return "SimpleType";
    case AstKind::ArrayType:      return "ArrayType";
    case AstKind::RecordType:     return "RecordType";
    case AstKind::Range:          return "Range";
    case AstKind::EnumeratedType: return "EnumeratedType";
    case AstKind::Assign:         return "Assign";
    case AstKind::ProcCall:       return "ProcCall";
    case AstKind::If:             return "If";
    case AstKind::Case:           return "Case";
    case AstKind::CaseBranch:     return "CaseBranch";
    case AstKind::While:          return "While";
    case AstKind::Repeat:         return "Repeat";
    case AstKind::For:            return "For";
    case AstKind::Compound:       return "Compound";
    case AstKind::BinOp:          return "BinOp";
    case AstKind::UnaryOp:        return "UnaryOp";
    case AstKind::Var:            return "Var";
    case AstKind::IntLit:         return "IntLit";
    case AstKind::RealLit:        return "RealLit";
    case AstKind::CharLit:        return "CharLit";
    case AstKind::StringLit:      return "StringLit";
    case AstKind::BoolLit:        return "BoolLit";
    case AstKind::ArrayAccess:    return "ArrayAccess";
    case AstKind::FieldAccess:    return "FieldAccess";
    }
    return "?";
}

const char* toString(BinOpKind op) {
    switch (op) {
    case BinOpKind::Add:    return "+";
    case BinOpKind::Sub:    return "-";
    case BinOpKind::Or:     return "OR";
    case BinOpKind::Mul:    return "*";
    case BinOpKind::Div:    return "/";
    case BinOpKind::IntDiv: return "DIV";
    case BinOpKind::Mod:    return "MOD";
    case BinOpKind::And:    return "AND";
    case BinOpKind::Eq:     return "==";
    case BinOpKind::Ne:     return "<>";
    case BinOpKind::Lt:     return "<";
    case BinOpKind::Le:     return "<=";
    case BinOpKind::Gt:     return ">";
    case BinOpKind::Ge:     return ">=";
    }
    return "?";
}

const char* toString(UnaryOpKind op) {
    switch (op) {
    case UnaryOpKind::Plus:  return "+";
    case UnaryOpKind::Minus: return "-";
    case UnaryOpKind::Not:   return "NOT";
    }
    return "?";
}

AstNode::AstNode(AstKind kind) : kind_(kind) {}
AstNode::~AstNode() = default;

void AstNode::printIndent(std::ostream& out, int depth) const {
    for (int i = 0; i < depth; ++i) out << "  ";
}

void AstNode::printAnnotations(std::ostream& out) const {
    bool opened = false;
    const auto emit = [&](const char* key, auto value) {
        out << (opened ? ", " : "  [") << key << '=' << value;
        opened = true;
    };

    if (inferredType != TypeKind::Unknown) emit("type", toString(inferredType));
    if (typeRef != NO_INDEX) emit("typeRef", typeRef);
    if (tabIdx != NO_INDEX) emit("tab", tabIdx);
    if (level != NO_INDEX) emit("level", level);
    if (line != NO_INDEX) emit("line", line);
    if (column != NO_INDEX) emit("col", column);
    if (opened) out << ']';
}

void AstNode::printSelf(std::ostream& out) const {
    out << kindName();
}

void AstNode::printChildren(std::ostream&, int) const {}

void AstNode::print(std::ostream& out, int depth) const {
    printIndent(out, depth);
    printSelf(out);
    printAnnotations(out);
    out << '\n';
    printChildren(out, depth + 1);
}

namespace {

std::string quote(const std::string& value) { // EDIT MARK
    return "'" + value + "'";
}

std::string exprToString(const AstNode* node); // EDIT MARK

std::string typeToString(const AstNode* node) { // EDIT MARK
    if (!node) return "?";

    if (const auto* simple = dynamic_cast<const SimpleTypeNode*>(node)) {
        return simple->name;
    }

    if (const auto* range = dynamic_cast<const RangeNode*>(node)) {
        return exprToString(range->low.get()) + ".." + exprToString(range->high.get());
    }

    if (const auto* array = dynamic_cast<const ArrayTypeNode*>(node)) {
        std::ostringstream out;
        out << "array[";
        for (std::size_t i = 0; i < array->indexTypes.size(); ++i) {
            if (i) out << ", ";
            out << typeToString(array->indexTypes[i].get());
        }
        out << "] of " << typeToString(array->elementType.get());
        return out.str();
    }

    if (const auto* enumerated = dynamic_cast<const EnumeratedTypeNode*>(node)) {
        std::ostringstream out;
        out << '(';
        for (std::size_t i = 0; i < enumerated->identifiers.size(); ++i) {
            if (i) out << ", ";
            out << enumerated->identifiers[i];
        }
        out << ')';
        return out.str();
    }

    if (dynamic_cast<const RecordTypeNode*>(node)) {
        return "record";
    }

    return node->kindName();
}

std::string exprToString(const AstNode* node) { // EDIT MARK
    if (!node) return "Empty";

    if (const auto* var = dynamic_cast<const VarNode*>(node)) {
        return "Var(" + quote(var->name) + ")";
    }

    if (const auto* intLit = dynamic_cast<const IntLitNode*>(node)) {
        return "Num(" + std::to_string(intLit->value) + ")";
    }

    if (const auto* realLit = dynamic_cast<const RealLitNode*>(node)) {
        std::ostringstream out;
        out << "Real(" << realLit->value << ')';
        return out.str();
    }

    if (const auto* charLit = dynamic_cast<const CharLitNode*>(node)) {
        return "Char('" + std::string(1, charLit->value) + "')";
    }

    if (const auto* stringLit = dynamic_cast<const StringLitNode*>(node)) {
        return "String(" + quote(stringLit->value) + ")";
    }

    if (const auto* boolLit = dynamic_cast<const BoolLitNode*>(node)) {
        return std::string("Bool(") + (boolLit->value ? "true" : "false") + ")";
    }

    if (const auto* unary = dynamic_cast<const UnaryOpNode*>(node)) {
        return "UnaryOp(op: '" + std::string(toString(unary->op)) +
               "', value: " + exprToString(unary->operand.get()) + ")";
    }

    if (const auto* binop = dynamic_cast<const BinOpNode*>(node)) {
        return "BinOp(op: '" + std::string(toString(binop->op)) +
               "', left: " + exprToString(binop->lhs.get()) +
               ", right: " + exprToString(binop->rhs.get()) + ")";
    }

    if (const auto* call = dynamic_cast<const ProcCallNode*>(node)) {
        std::ostringstream out;
        out << "ProcedureCall(name: " << quote(call->name) << ", args: [";
        for (std::size_t i = 0; i < call->args.size(); ++i) {
            if (i) out << ", ";
            out << exprToString(call->args[i].get());
        }
        out << "])";
        return out.str();
    }

    if (const auto* arrayAccess = dynamic_cast<const ArrayAccessNode*>(node)) {
        std::ostringstream out;
        out << "ArrayAccess(base: " << exprToString(arrayAccess->base.get()) << ", indices: [";
        for (std::size_t i = 0; i < arrayAccess->indices.size(); ++i) {
            if (i) out << ", ";
            out << exprToString(arrayAccess->indices[i].get());
        }
        out << "])";
        return out.str();
    }

    if (const auto* fieldAccess = dynamic_cast<const FieldAccessNode*>(node)) {
        return "FieldAccess(base: " + exprToString(fieldAccess->base.get()) +
               ", field: " + quote(fieldAccess->field) + ")";
    }

    return node->kindName();
}

void printNodePretty(std::ostream& out, const AstNode* node, const std::string& prefix, bool last); // EDIT MARK

void printChildrenPretty(std::ostream& out,
                         const std::vector<const AstNode*>& nodes,
                         const std::string& prefix) { // EDIT MARK
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        printNodePretty(out, nodes[i], prefix, i + 1 == nodes.size());
    }
}

void printDeclSection(std::ostream& out, const DeclarationNode* decl, const std::string& prefix, bool last) { // EDIT MARK
    out << prefix << (last ? "\\-- " : "+-- ") << "Declarations\n";
    const std::string childPrefix = prefix + (last ? "    " : "|   ");

    std::vector<const AstNode*> decls;
    if (decl) {
        for (const auto& item : decl->constDecls) decls.push_back(item.get());
        for (const auto& item : decl->typeDecls) decls.push_back(item.get());
        for (const auto& item : decl->varDecls) decls.push_back(item.get());
        for (const auto& item : decl->subprogDecls) decls.push_back(item.get());
    }
    printChildrenPretty(out, decls, childPrefix);
}

void printBlockSection(std::ostream& out, const CompoundNode* block, const std::string& prefix, bool last) { // EDIT MARK
    out << prefix << (last ? "\\-- " : "+-- ") << "Block\n";
    const std::string childPrefix = prefix + (last ? "    " : "|   ");

    std::vector<const AstNode*> statements;
    if (block) {
        for (const auto& statement : block->statements) statements.push_back(statement.get());
    }
    printChildrenPretty(out, statements, childPrefix);
}

void printNodePretty(std::ostream& out, const AstNode* node, const std::string& prefix, bool last) { // EDIT MARK
    if (!node) return;

    out << prefix << (last ? "\\-- " : "+-- ");

    if (const auto* constDecl = dynamic_cast<const ConstDeclNode*>(node)) {
        out << "ConstDecl(name: " << quote(constDecl->name)
            << ", value: " << exprToString(constDecl->value.get()) << ")\n";
        return;
    }

    if (const auto* typeDecl = dynamic_cast<const TypeDeclNode*>(node)) {
        out << "TypeDecl(name: " << quote(typeDecl->name)
            << ", type: " << quote(typeToString(typeDecl->typeExpr.get())) << ")\n";
        return;
    }

    if (const auto* varDecl = dynamic_cast<const VarDeclNode*>(node)) {
        out << "VarDecl(name: " << quote(varDecl->name)
            << ", type: " << quote(typeToString(varDecl->typeExpr.get())) << ")\n";
        return;
    }

    if (const auto* assign = dynamic_cast<const AssignNode*>(node)) {
        out << "Assign(target: " << exprToString(assign->target.get())
            << ", value: " << exprToString(assign->value.get()) << ")\n";
        return;
    }

    if (const auto* call = dynamic_cast<const ProcCallNode*>(node)) {
        out << exprToString(call) << '\n';
        return;
    }

    if (const auto* proc = dynamic_cast<const ProcDeclNode*>(node)) {
        out << "ProcedureDecl(name: " << quote(proc->name) << ")\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        if (proc->block) {
            printDeclSection(out, proc->block->declaration.get(), childPrefix, false);
            printBlockSection(out, proc->block->statements.get(), childPrefix, true);
        }
        return;
    }

    if (const auto* func = dynamic_cast<const FuncDeclNode*>(node)) {
        out << "FunctionDecl(name: " << quote(func->name)
            << ", returnType: " << quote(typeToString(func->returnType.get())) << ")\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        if (func->block) {
            printDeclSection(out, func->block->declaration.get(), childPrefix, false);
            printBlockSection(out, func->block->statements.get(), childPrefix, true);
        }
        return;
    }

    if (const auto* compound = dynamic_cast<const CompoundNode*>(node)) {
        out << "Block\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        std::vector<const AstNode*> statements;
        for (const auto& statement : compound->statements) statements.push_back(statement.get());
        printChildrenPretty(out, statements, childPrefix);
        return;
    }

    if (const auto* ifNode = dynamic_cast<const IfNode*>(node)) {
        out << "If(condition: " << exprToString(ifNode->condition.get()) << ")\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        std::vector<const AstNode*> branches = {ifNode->thenStmt.get()};
        if (ifNode->elseStmt) branches.push_back(ifNode->elseStmt.get());
        printChildrenPretty(out, branches, childPrefix);
        return;
    }

    if (const auto* whileNode = dynamic_cast<const WhileNode*>(node)) {
        out << "While(condition: " << exprToString(whileNode->condition.get()) << ")\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        printNodePretty(out, whileNode->body.get(), childPrefix, true);
        return;
    }

    if (const auto* repeatNode = dynamic_cast<const RepeatNode*>(node)) {
        out << "Repeat(until: " << exprToString(repeatNode->condition.get()) << ")\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        std::vector<const AstNode*> body;
        for (const auto& statement : repeatNode->body) body.push_back(statement.get());
        printChildrenPretty(out, body, childPrefix);
        return;
    }

    if (const auto* forNode = dynamic_cast<const ForNode*>(node)) {
        out << "For(var: " << quote(forNode->controlVar)
            << ", start: " << exprToString(forNode->startExpr.get())
            << ", direction: " << (forNode->downto ? "downto" : "to")
            << ", end: " << exprToString(forNode->endExpr.get()) << ")\n";
        const std::string childPrefix = prefix + (last ? "    " : "|   ");
        printNodePretty(out, forNode->body.get(), childPrefix, true);
        return;
    }

    out << exprToString(node) << '\n';
}

}

void printAst(std::ostream& out, const AstNode* root) { // EDIT MARK
    if (!root) return;

    if (const auto* program = dynamic_cast<const ProgramNode*>(root)) {
        out << "ProgramNode(name: " << quote(program->name) << ")\n";
        printDeclSection(out, program->declaration.get(), "", false);
        printBlockSection(out, program->statements.get(), "", true);
        return;
    }

    printNodePretty(out, root, "", true);
}

static void printLabeled(std::ostream& out, int depth, const char* label, const AstNode* node) {
    if (!node) return;
    for (int i = 0; i < depth; ++i) out << "  ";
    out << '[' << label << "]\n";
    node->print(out, depth + 1);
}

static void printList(std::ostream& out, int depth, const char* label, const std::vector<AstPtr>& nodes) {
    if (nodes.empty()) return;
    for (int i = 0; i < depth; ++i) out << "  ";
    out << '[' << label << "]\n";
    for (const auto& node : nodes) {
        if (node) node->print(out, depth + 1);
    }
}

DeclarationNode::DeclarationNode() : AstNode(AstKind::DeclPart) {} // EDIT MARK

void DeclarationNode::printChildren(std::ostream& out, int depth) const { // EDIT MARK
    printList(out, depth, "consts", constDecls);
    printList(out, depth, "types", typeDecls);
    printList(out, depth, "vars", varDecls);
    printList(out, depth, "subprograms", subprogDecls);
}

BlockNode::BlockNode() : AstNode(AstKind::Block) {}

void BlockNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "declarations", declaration.get());
    printLabeled(out, depth, "body", statements.get());
}

ProgramNode::ProgramNode() : AstNode(AstKind::Program) {}

void ProgramNode::printSelf(std::ostream& out) const {
    out << "Program " << name;
}

void ProgramNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "declarations", declaration.get());
    printLabeled(out, depth, "body", statements.get());
}

ConstDeclNode::ConstDeclNode() : AstNode(AstKind::ConstDecl) {}

void ConstDeclNode::printSelf(std::ostream& out) const {
    out << "ConstDecl " << name;
}

void ConstDeclNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "value", value.get());
}

TypeDeclNode::TypeDeclNode() : AstNode(AstKind::TypeDecl) {}

void TypeDeclNode::printSelf(std::ostream& out) const {
    out << "TypeDecl " << name;
}

void TypeDeclNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "type", typeExpr.get());
}

VarDeclNode::VarDeclNode() : AstNode(AstKind::VarDecl) {}

void VarDeclNode::printSelf(std::ostream& out) const {
    out << "VarDecl " << name;
}

void VarDeclNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "type", typeExpr.get());
}

static void printParams(std::ostream& out, int depth, const std::vector<FormalParam>& params) {
    if (params.empty()) return;
    for (int i = 0; i < depth; ++i) out << "  ";
    out << "[params]\n";

    for (const auto& param : params) {
        for (int i = 0; i < depth + 1; ++i) out << "  ";
        if (param.byReference) out << "var ";
        out << param.name << '\n';
        if (param.typeExpr) param.typeExpr->print(out, depth + 2);
    }
}

ProcDeclNode::ProcDeclNode() : AstNode(AstKind::ProcDecl) {}

void ProcDeclNode::printSelf(std::ostream& out) const {
    out << "ProcDecl " << name;
}

void ProcDeclNode::printChildren(std::ostream& out, int depth) const {
    printParams(out, depth, params);
    if (block) block->print(out, depth);
}

FuncDeclNode::FuncDeclNode() : AstNode(AstKind::FuncDecl) {}

void FuncDeclNode::printSelf(std::ostream& out) const {
    out << "FuncDecl " << name;
}

void FuncDeclNode::printChildren(std::ostream& out, int depth) const {
    printParams(out, depth, params);
    printLabeled(out, depth, "returnType", returnType.get());
    if (block) block->print(out, depth);
}

SimpleTypeNode::SimpleTypeNode() : AstNode(AstKind::SimpleType) {}

void SimpleTypeNode::printSelf(std::ostream& out) const {
    out << "SimpleType " << name;
}

RangeNode::RangeNode() : AstNode(AstKind::Range) {}

void RangeNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "low", low.get());
    printLabeled(out, depth, "high", high.get());
}

ArrayTypeNode::ArrayTypeNode() : AstNode(AstKind::ArrayType) {}

void ArrayTypeNode::printChildren(std::ostream& out, int depth) const {
    printList(out, depth, "indexTypes", indexTypes);
    printLabeled(out, depth, "element", elementType.get());
}

RecordTypeNode::RecordTypeNode() : AstNode(AstKind::RecordType) {}

void RecordTypeNode::printChildren(std::ostream& out, int depth) const {
    for (const auto& field : fields) {
        for (int i = 0; i < depth; ++i) out << "  ";
        out << "[field] ";
        for (std::size_t i = 0; i < field.names.size(); ++i) {
            if (i) out << ", ";
            out << field.names[i];
        }
        out << '\n';
        if (field.typeExpr) field.typeExpr->print(out, depth + 1);
    }
}

EnumeratedTypeNode::EnumeratedTypeNode() : AstNode(AstKind::EnumeratedType) {}

void EnumeratedTypeNode::printSelf(std::ostream& out) const {
    out << "EnumeratedType (";
    for (std::size_t i = 0; i < identifiers.size(); ++i) {
        if (i) out << ", ";
        out << identifiers[i];
    }
    out << ')';
}

AssignNode::AssignNode() : AstNode(AstKind::Assign) {}

void AssignNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "target", target.get());
    printLabeled(out, depth, "value", value.get());
}

ProcCallNode::ProcCallNode() : AstNode(AstKind::ProcCall) {}

void ProcCallNode::printSelf(std::ostream& out) const {
    out << "ProcCall " << name;
}

void ProcCallNode::printChildren(std::ostream& out, int depth) const {
    printList(out, depth, "args", args);
}

IfNode::IfNode() : AstNode(AstKind::If) {}

void IfNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "condition", condition.get());
    printLabeled(out, depth, "then", thenStmt.get());
    printLabeled(out, depth, "else", elseStmt.get());
}

CaseBranchNode::CaseBranchNode() : AstNode(AstKind::CaseBranch) {}

void CaseBranchNode::printChildren(std::ostream& out, int depth) const {
    printList(out, depth, "labels", labels);
    printLabeled(out, depth, "statement", statement.get());
}

CaseNode::CaseNode() : AstNode(AstKind::Case) {}

void CaseNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "selector", selector.get());
    for (const auto& branch : branches) {
        if (branch) branch->print(out, depth);
    }
}

WhileNode::WhileNode() : AstNode(AstKind::While) {}

void WhileNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "condition", condition.get());
    printLabeled(out, depth, "body", body.get());
}

RepeatNode::RepeatNode() : AstNode(AstKind::Repeat) {}

void RepeatNode::printChildren(std::ostream& out, int depth) const {
    printList(out, depth, "body", body);
    printLabeled(out, depth, "until", condition.get());
}

ForNode::ForNode() : AstNode(AstKind::For) {}

void ForNode::printSelf(std::ostream& out) const {
    out << "For " << controlVar << (downto ? " downto" : " to");
}

void ForNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "start", startExpr.get());
    printLabeled(out, depth, "end", endExpr.get());
    printLabeled(out, depth, "body", body.get());
}

CompoundNode::CompoundNode() : AstNode(AstKind::Compound) {}

void CompoundNode::printChildren(std::ostream& out, int depth) const {
    for (const auto& statement : statements) {
        if (statement) statement->print(out, depth);
    }
}

BinOpNode::BinOpNode() : AstNode(AstKind::BinOp) {}

void BinOpNode::printSelf(std::ostream& out) const {
    out << "BinOp " << toString(op);
}

void BinOpNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "lhs", lhs.get());
    printLabeled(out, depth, "rhs", rhs.get());
}

UnaryOpNode::UnaryOpNode() : AstNode(AstKind::UnaryOp) {}

void UnaryOpNode::printSelf(std::ostream& out) const {
    out << "UnaryOp " << toString(op);
}

void UnaryOpNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "operand", operand.get());
}

VarNode::VarNode() : AstNode(AstKind::Var) {}

void VarNode::printSelf(std::ostream& out) const {
    out << "Var " << name;
}

IntLitNode::IntLitNode() : AstNode(AstKind::IntLit) {}

void IntLitNode::printSelf(std::ostream& out) const {
    out << "IntLit " << value;
}

RealLitNode::RealLitNode() : AstNode(AstKind::RealLit) {}

void RealLitNode::printSelf(std::ostream& out) const {
    out << "RealLit " << value;
}

CharLitNode::CharLitNode() : AstNode(AstKind::CharLit) {}

void CharLitNode::printSelf(std::ostream& out) const {
    out << "CharLit '" << value << "'";
}

StringLitNode::StringLitNode() : AstNode(AstKind::StringLit) {}

void StringLitNode::printSelf(std::ostream& out) const {
    out << "StringLit \"" << value << "\"";
}

BoolLitNode::BoolLitNode() : AstNode(AstKind::BoolLit) {}

void BoolLitNode::printSelf(std::ostream& out) const {
    out << "BoolLit " << (value ? "true" : "false");
}

ArrayAccessNode::ArrayAccessNode() : AstNode(AstKind::ArrayAccess) {}

void ArrayAccessNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "base", base.get());
    printList(out, depth, "indices", indices);
}

FieldAccessNode::FieldAccessNode() : AstNode(AstKind::FieldAccess) {}

void FieldAccessNode::printSelf(std::ostream& out) const {
    out << "FieldAccess ." << field;
}

void FieldAccessNode::printChildren(std::ostream& out, int depth) const {
    printLabeled(out, depth, "base", base.get());
}

}
