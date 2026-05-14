#include "ast_nodes.hpp"

#include <ostream>

namespace semantic {

const char* toString(AstKind kind) {
    switch (kind) {
    case AstKind::Program:        return "Program";
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

void printAst(std::ostream& out, const AstNode* root) {
    if (root) root->print(out, 0);
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

BlockNode::BlockNode() : AstNode(AstKind::Block) {}

void BlockNode::printChildren(std::ostream& out, int depth) const {
    printList(out, depth, "consts", constDecls);
    printList(out, depth, "types", typeDecls);
    printList(out, depth, "vars", varDecls);
    printList(out, depth, "subprograms", subprogDecls);
    printList(out, depth, "body", statements);
}

ProgramNode::ProgramNode() : AstNode(AstKind::Program) {}

void ProgramNode::printSelf(std::ostream& out) const {
    out << "Program " << name;
    if (!programParams.empty()) {
        out << '(';
        for (std::size_t i = 0; i < programParams.size(); ++i) {
            if (i) out << ", ";
            out << programParams[i];
        }
        out << ')';
    }
}

void ProgramNode::printChildren(std::ostream& out, int depth) const {
    if (block) block->print(out, depth);
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
    out << "VarDecl ";
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i) out << ", ";
        out << names[i];
    }
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
        out << '(';
        for (std::size_t i = 0; i < param.names.size(); ++i) {
            if (i) out << ", ";
            out << param.names[i];
        }
        out << ")\n";
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
