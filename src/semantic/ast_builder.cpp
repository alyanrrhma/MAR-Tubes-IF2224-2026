#include "ast_builder.hpp"

#include <cctype>
#include <stdexcept>

namespace {

const auto& children(const parse_tree::NodePtr& node) { 
    return node->getChildren();
}

bool isLabel(const parse_tree::NodePtr& node, const std::string& label) { 
    return node && node->getLabel() == label;
}

bool isIdent(const parse_tree::NodePtr& node) { 
    return isLabel(node, "ident");
}

void annotate(semantic::AstNode* ast, const parse_tree::NodePtr& node) { 
    if (!ast || !node) return;
    ast->line = node->getLine();
    ast->column = node->getColumn();
}

std::string lower(std::string value) { 
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

semantic::AstPtr makeSimpleType(const std::string& name, const parse_tree::NodePtr& node) { 
    auto type = std::make_unique<semantic::SimpleTypeNode>();
    type->name = name;
    annotate(type.get(), node);
    return type;
}

semantic::AstPtr makeVar(const std::string& name, const parse_tree::NodePtr& node) { 
    auto var = std::make_unique<semantic::VarNode>();
    var->name = name;
    annotate(var.get(), node);
    return var;
}

}

semantic::AstPtr AstBuilder::build(const parse_tree::NodePtr& root) { 
    if (!root) return nullptr;
    if (root->getLabel() != "program") {
        throw std::runtime_error("AST builder expects a <program> parse-tree root");
    }
    return visit_Program(root);
}

std::unique_ptr<semantic::ProgramNode> AstBuilder::visit_Program(const parse_tree::NodePtr& node) { 
    auto result = std::make_unique<semantic::ProgramNode>();
    annotate(result.get(), node);
    result->name = visit_ProgramHeader(children(node).at(0));
    result->declaration = visit_DeclarationPart(children(node).at(1));
    result->statements = visit_CompoundStatement(children(node).at(2));
    return result;
}

std::string AstBuilder::visit_ProgramHeader(const parse_tree::NodePtr& node) { 
    return children(node).at(1)->getValue();
}

std::string AstBuilder::visit_ProgramHeading(const parse_tree::NodePtr& node) { 
    return visit_ProgramHeader(node);
}

std::vector<std::string> AstBuilder::visit_IdentifierList(const parse_tree::NodePtr& node) { 
    std::vector<std::string> idents;
    for (const auto& child : children(node)) {
        if (isIdent(child)) idents.push_back(child->getValue());
    }
    return idents;
}

std::unique_ptr<semantic::BlockNode> AstBuilder::visit_Block(const parse_tree::NodePtr& node) { 
    auto result = std::make_unique<semantic::BlockNode>();
    annotate(result.get(), node);
    result->declaration = visit_DeclarationPart(children(node).at(0));
    result->statements = visit_CompoundStatement(children(node).at(1));
    return result;
}

std::unique_ptr<semantic::DeclarationNode> AstBuilder::visit_DeclarationPart(const parse_tree::NodePtr& node) { 
    auto result = std::make_unique<semantic::DeclarationNode>();
    annotate(result.get(), node);
    for (const auto& child : children(node)) {
        if (isLabel(child, "const-declaration")) {
            auto decls = visit_ConstDeclaration(child);
            for (auto& decl : decls) {
                result->declarations.push_back(std::move(decl));
            }
        } else if (isLabel(child, "type-declaration")) {
            auto decls = visit_TypeDeclaration(child);
            for (auto& decl : decls) {
                result->declarations.push_back(std::move(decl));
            }
        } else if (isLabel(child, "var-declaration")) {
            auto decls = visit_VarDeclaration(child);
            for (auto& decl : decls) {
                result->declarations.push_back(std::move(decl));
            }
        } else if (isLabel(child, "subprogram-declaration-part")) {
            auto decls = visit_SubprogramDeclarationPart(child);
            for (auto& decl : decls) {
                result->declarations.push_back(std::move(decl));
            }
        }
    }
    return result;
}

std::vector<semantic::AstPtr> AstBuilder::visit_ConstDeclaration(const parse_tree::NodePtr& node) { 
    std::vector<semantic::AstPtr> decls;
    for (const auto& child : children(node)) {
        if (isLabel(child, "const-definition")) decls.push_back(visit_ConstDefinition(child));
    }
    return decls;
}

std::vector<semantic::AstPtr> AstBuilder::visit_ConstantDeclaration(const parse_tree::NodePtr& node) { 
    return visit_ConstDeclaration(node);
}

std::unique_ptr<semantic::ConstDeclNode> AstBuilder::visit_ConstDefinition(const parse_tree::NodePtr& node) { 
    auto constant = std::make_unique<semantic::ConstDeclNode>();
    annotate(constant.get(), node);
    constant->name = children(node).at(0)->getValue();
    constant->value = visit_Constant(children(node).at(2));
    return constant;
}

std::unique_ptr<semantic::ConstDeclNode> AstBuilder::visit_ConstantDefinition(const parse_tree::NodePtr& node) { 
    return visit_ConstDefinition(node);
}

std::vector<semantic::AstPtr> AstBuilder::visit_TypeDeclaration(const parse_tree::NodePtr& node) { 
    std::vector<semantic::AstPtr> decls;
    for (const auto& child : children(node)) {
        if (isLabel(child, "type-definition")) decls.push_back(visit_TypeDefinition(child));
    }
    return decls;
}

std::unique_ptr<semantic::TypeDeclNode> AstBuilder::visit_TypeDefinition(const parse_tree::NodePtr& node) { 
    auto decl = std::make_unique<semantic::TypeDeclNode>();
    annotate(decl.get(), node);
    decl->name = children(node).at(0)->getValue();
    decl->typeExpr = visit_Type(children(node).at(2));
    return decl;
}

std::vector<semantic::AstPtr> AstBuilder::visit_VarDeclaration(const parse_tree::NodePtr& node) { 
    std::vector<semantic::AstPtr> decls;
    for (std::size_t i = 1; i + 2 < children(node).size(); i += 4) {
        if (!isLabel(children(node).at(i), "identifier-list")) continue;
        for (const auto& name : visit_IdentifierList(children(node).at(i))) {
            auto decl = std::make_unique<semantic::VarDeclNode>();
            annotate(decl.get(), children(node).at(i));
            decl->name = name;
            decl->typeExpr = visit_Type(children(node).at(i + 2));
            decls.push_back(std::move(decl));
        }
    }
    return decls;
}

std::vector<semantic::AstPtr> AstBuilder::visit_VariableDeclaration(const parse_tree::NodePtr& node) { 
    return visit_VarDeclaration(node);
}

semantic::AstPtr AstBuilder::visit_Type(const parse_tree::NodePtr& node) { 
    const auto& first = children(node).at(0);
    if (isLabel(first, "array-type")) return visit_ArrayType(first);
    if (isLabel(first, "record-type")) return visit_RecordType(first);
    if (isLabel(first, "enumerated")) {
        auto type = std::make_unique<semantic::EnumeratedTypeNode>();
        annotate(type.get(), first);
        type->identifiers = visit_Enumerated(first);
        return type;
    }
    if (isLabel(first, "range")) return visit_Range(first);
    return makeSimpleType(first->getValue(), first);
}

semantic::AstPtr AstBuilder::visit_TypeDenoter(const parse_tree::NodePtr& node) { 
    return visit_Type(node);
}

std::unique_ptr<semantic::ArrayTypeNode> AstBuilder::visit_ArrayType(const parse_tree::NodePtr& node) { 
    auto arrayType = std::make_unique<semantic::ArrayTypeNode>();
    annotate(arrayType.get(), node);
    for (const auto& child : children(node)) {
        if (isLabel(child, "range")) arrayType->indexTypes.push_back(visit_Range(child));
    }
    arrayType->elementType = visit_Type(children(node).at(children(node).size() - 1));
    return arrayType;
}

std::unique_ptr<semantic::RangeNode> AstBuilder::visit_Range(const parse_tree::NodePtr& node) { 
    auto rangeNode = std::make_unique<semantic::RangeNode>();
    annotate(rangeNode.get(), node);
    rangeNode->low = visit_Constant(children(node).at(0));
    rangeNode->high = visit_Constant(children(node).at(3));
    return rangeNode;
}

std::vector<std::string> AstBuilder::visit_Enumerated(const parse_tree::NodePtr& node) { 
    return visit_IdentifierList(children(node).at(1));
}

std::unique_ptr<semantic::RecordTypeNode> AstBuilder::visit_RecordType(const parse_tree::NodePtr& node) { 
    return visit_FieldList(children(node).at(1));
}

std::unique_ptr<semantic::RecordTypeNode> AstBuilder::visit_FieldList(const parse_tree::NodePtr& node) { 
    auto record = std::make_unique<semantic::RecordTypeNode>();
    annotate(record.get(), node);
    for (const auto& child : children(node)) {
        if (isLabel(child, "field-part")) record->fields.push_back(visit_FieldPart(child));
    }
    return record;
}

semantic::RecordTypeNode::FieldSection AstBuilder::visit_FieldPart(const parse_tree::NodePtr& node) { 
    semantic::RecordTypeNode::FieldSection field;
    field.names = visit_IdentifierList(children(node).at(0));
    field.typeExpr = visit_Type(children(node).at(2));
    return field;
}

std::vector<semantic::AstPtr> AstBuilder::visit_SubprogramDeclarationPart(const parse_tree::NodePtr& node) { 
    std::vector<semantic::AstPtr> subs;
    for (const auto& child : children(node)) {
        if (isLabel(child, "subprogram-declaration")) subs.push_back(visit_SubprogramDeclaration(child));
    }
    return subs;
}

semantic::AstPtr AstBuilder::visit_SubprogramDeclaration(const parse_tree::NodePtr& node) { 
    if (isLabel(children(node).at(0), "procedure-declaration")) {
        return visit_ProcedureDeclaration(children(node).at(0));
    }
    return visit_FunctionDeclaration(children(node).at(0));
}

std::unique_ptr<semantic::ProcDeclNode> AstBuilder::visit_ProcedureDeclaration(const parse_tree::NodePtr& node) { 
    auto decl = std::make_unique<semantic::ProcDeclNode>();
    annotate(decl.get(), node);
    auto heading = visit_ProcedureHeading(children(node).at(0));
    decl->name = std::move(heading.first);
    decl->params = std::move(heading.second);
    decl->block = visit_Block(children(node).at(2));
    return decl;
}

std::unique_ptr<semantic::FuncDeclNode> AstBuilder::visit_FunctionDeclaration(const parse_tree::NodePtr& node) { 
    auto decl = std::make_unique<semantic::FuncDeclNode>();
    annotate(decl.get(), node);
    auto heading = visit_FunctionHeading(children(node).at(0));
    decl->name = std::move(std::get<0>(heading));
    decl->params = std::move(std::get<1>(heading));
    decl->returnType = makeSimpleType(std::get<2>(heading), children(node).at(0));

    const auto savedFunctionName = currentFunctionName_;
    currentFunctionName_ = decl->name;
    decl->block = visit_Block(children(node).at(2));
    currentFunctionName_ = savedFunctionName;
    return decl;
}

std::pair<std::string, std::vector<semantic::FormalParam>> AstBuilder::visit_ProcedureHeading(const parse_tree::NodePtr& node) { 
    std::string name = children(node).at(1)->getValue();
    if (children(node).size() == 2) return std::make_pair(std::move(name), std::vector<semantic::FormalParam>{});
    return std::make_pair(std::move(name), visit_FormalParameterList(children(node).at(2)));
}

std::tuple<std::string, std::vector<semantic::FormalParam>, std::string> AstBuilder::visit_FunctionHeading(const parse_tree::NodePtr& node) { 
    std::string name = children(node).at(1)->getValue();
    std::string returnType = children(node).back()->getValue();
    if (children(node).size() == 4) {
        return std::make_tuple(std::move(name), std::vector<semantic::FormalParam>{}, std::move(returnType));
    }
    return std::make_tuple(std::move(name), visit_FormalParameterList(children(node).at(2)), std::move(returnType));
}

std::vector<semantic::FormalParam> AstBuilder::visit_FormalParameterList(const parse_tree::NodePtr& node) { 
    std::vector<semantic::FormalParam> params;
    for (const auto& child : children(node)) {
        if (!isLabel(child, "parameter-group")) continue;
        auto names = visit_IdentifierList(children(child).at(0));
        for (const auto& name : names) {
            auto param = visit_ParameterGroup(child);
            param.name = name;
            params.push_back(std::move(param));
        }
    }
    return params;
}

semantic::FormalParam AstBuilder::visit_ParameterGroup(const parse_tree::NodePtr& node) { 
    semantic::FormalParam param;
    auto names = visit_IdentifierList(children(node).at(0));
    if (!names.empty()) param.name = names.front();
    if (isLabel(children(node).at(2), "array-type")) {
        param.typeExpr = visit_ArrayType(children(node).at(2));
    } else {
        param.typeExpr = makeSimpleType(children(node).at(2)->getValue(), children(node).at(2));
    }
    return param;
}

semantic::FormalParam AstBuilder::visit_FormalParameterSection(const parse_tree::NodePtr& node) { 
    return visit_ParameterGroup(node);
}

std::unique_ptr<semantic::CompoundNode> AstBuilder::visit_CompoundStatement(const parse_tree::NodePtr& node) { 
    return visit_StatementList(children(node).at(1));
}

std::unique_ptr<semantic::CompoundNode> AstBuilder::visit_StatementList(const parse_tree::NodePtr& node) { 
    auto compound = std::make_unique<semantic::CompoundNode>();
    annotate(compound.get(), node);
    for (const auto& child : children(node)) {
        if (!isLabel(child, "statement")) continue;
        auto stmt = visit_Statement(child);
        if (stmt) compound->statements.push_back(std::move(stmt));
    }
    return compound;
}

semantic::AstPtr AstBuilder::visit_Statement(const parse_tree::NodePtr& node) { 
    if (children(node).empty()) return nullptr;
    const auto& child = children(node).at(0);
    if (isLabel(child, "assignment-statement")) return visit_AssignmentStatement(child);
    if (isLabel(child, "procedure/function-call")) return visit_ProcedureFunctionCall(child);
    if (isLabel(child, "compound-statement")) return visit_CompoundStatement(child);
    if (isLabel(child, "if-statement")) return visit_IfStatement(child);
    if (isLabel(child, "case-statement")) return visit_CaseStatement(child);
    if (isLabel(child, "repeat-statement")) return visit_RepeatStatement(child);
    if (isLabel(child, "while-statement")) return visit_WhileStatement(child);
    if (isLabel(child, "for-statement")) return visit_ForStatement(child);
    return visit_Empty(child);
}

semantic::AstPtr AstBuilder::visit_AssignmentStatement(const parse_tree::NodePtr& node) { 
    auto target = visit_Variable(children(node).at(0));
    auto value = visit_Expression(children(node).at(2));

    if (!currentFunctionName_.empty()) {
        if (auto* varNode = dynamic_cast<semantic::VarNode*>(target.get())) {
            if (varNode->name == currentFunctionName_) {
                auto ret = std::make_unique<semantic::ReturnNode>();
                annotate(ret.get(), node);
                ret->value = std::move(value);
                return ret;
            }
        }
    }

    auto assign = std::make_unique<semantic::AssignNode>();
    annotate(assign.get(), node);
    assign->target = std::move(target);
    assign->value = std::move(value);
    return assign;
}

std::unique_ptr<semantic::ProcCallNode> AstBuilder::visit_ProcedureFunctionCall(const parse_tree::NodePtr& node) { 
    auto call = std::make_unique<semantic::ProcCallNode>();
    annotate(call.get(), node);
    if (isLabel(children(node).at(0), "variable")) {
        auto var = visit_Variable(children(node).at(0));
        if (auto* varNode = dynamic_cast<semantic::VarNode*>(var.get())) call->name = varNode->name;
        else call->args.push_back(std::move(var));
        return call;
    }
    call->name = children(node).at(0)->getValue();
    for (const auto& child : children(node)) {
        if (isLabel(child, "parameter-list")) call->args = visit_ParameterList(child);
    }
    return call;
}

std::unique_ptr<semantic::ProcCallNode> AstBuilder::visit_ProcedureCallStatement(const parse_tree::NodePtr& node) { 
    return visit_ProcedureFunctionCall(node);
}

std::unique_ptr<semantic::IfNode> AstBuilder::visit_IfStatement(const parse_tree::NodePtr& node) { 
    auto stmt = std::make_unique<semantic::IfNode>();
    annotate(stmt.get(), node);
    stmt->condition = visit_Expression(children(node).at(1));
    stmt->thenStmt = visit_Statement(children(node).at(3));
    if (children(node).size() > 4) stmt->elseStmt = visit_Statement(children(node).at(5));
    return stmt;
}

std::unique_ptr<semantic::CaseNode> AstBuilder::visit_CaseStatement(const parse_tree::NodePtr& node) { 
    auto stmt = std::make_unique<semantic::CaseNode>();
    annotate(stmt.get(), node);
    stmt->selector = visit_Expression(children(node).at(1));
    for (const auto& child : children(node)) {
        if (isLabel(child, "case-block")) stmt->branches.push_back(visit_CaseBlock(child));
    }
    return stmt;
}

std::unique_ptr<semantic::CaseBranchNode> AstBuilder::visit_CaseBlock(const parse_tree::NodePtr& node) { 
    auto branch = std::make_unique<semantic::CaseBranchNode>();
    annotate(branch.get(), node);
    for (const auto& child : children(node)) {
        if (isLabel(child, "constant")) branch->labels.push_back(visit_Constant(child));
        else if (isLabel(child, "statement")) branch->statement = visit_Statement(child);
    }
    return branch;
}

std::unique_ptr<semantic::RepeatNode> AstBuilder::visit_RepeatStatement(const parse_tree::NodePtr& node) { 
    auto stmt = std::make_unique<semantic::RepeatNode>();
    annotate(stmt.get(), node);
    stmt->body = visit_StatementList(children(node).at(1));
    stmt->condition = visit_Expression(children(node).at(3));
    return stmt;
}

std::unique_ptr<semantic::WhileNode> AstBuilder::visit_WhileStatement(const parse_tree::NodePtr& node) { 
    auto stmt = std::make_unique<semantic::WhileNode>();
    annotate(stmt.get(), node);
    stmt->condition = visit_Expression(children(node).at(1));
    stmt->body = visit_CompoundStatement(children(node).at(3));
    return stmt;
}

std::unique_ptr<semantic::ForNode> AstBuilder::visit_ForStatement(const parse_tree::NodePtr& node) { 
    auto stmt = std::make_unique<semantic::ForNode>();
    annotate(stmt.get(), node);
    stmt->controlVar = children(node).at(1)->getValue();
    stmt->startExpr = visit_Expression(children(node).at(3));
    stmt->downto = isLabel(children(node).at(4), "downtosy");
    stmt->endExpr = visit_Expression(children(node).at(5));
    stmt->body = visit_CompoundStatement(children(node).at(7));
    return stmt;
}

std::vector<semantic::AstPtr> AstBuilder::visit_ParameterList(const parse_tree::NodePtr& node) { 
    std::vector<semantic::AstPtr> args;
    for (const auto& child : children(node)) {
        if (isLabel(child, "expression")) args.push_back(visit_Expression(child));
    }
    return args;
}

semantic::AstPtr AstBuilder::visit_Expression(const parse_tree::NodePtr& node) { 
    if (children(node).size() == 1) return visit_SimpleExpression(children(node).at(0));
    auto binop = std::make_unique<semantic::BinOpNode>();
    annotate(binop.get(), node);
    binop->op = visit_RelationalOperator(children(node).at(1));
    binop->lhs = visit_SimpleExpression(children(node).at(0));
    binop->rhs = visit_SimpleExpression(children(node).at(2));
    return binop;
}

semantic::BinOpKind AstBuilder::visit_RelationalOperator(const parse_tree::NodePtr& node) { 
    auto term = children(node).at(0)->getLabel();
    if (term == "neq") return semantic::BinOpKind::Ne;
    if (term == "gtr") return semantic::BinOpKind::Gt;
    if (term == "geq") return semantic::BinOpKind::Ge;
    if (term == "lss") return semantic::BinOpKind::Lt;
    if (term == "leq") return semantic::BinOpKind::Le;
    return semantic::BinOpKind::Eq;
}

semantic::AstPtr AstBuilder::visit_SimpleExpression(const parse_tree::NodePtr& node) { 
    std::size_t index = 0;
    semantic::UnaryOpKind unary = semantic::UnaryOpKind::Plus;
    bool hasUnary = false;
    if (isLabel(children(node).at(0), "plus") || isLabel(children(node).at(0), "minus")) {
        hasUnary = true;
        unary = isLabel(children(node).at(0), "minus") ? semantic::UnaryOpKind::Minus : semantic::UnaryOpKind::Plus;
        index = 1;
    }

    auto expr = visit_Term(children(node).at(index));
    if (hasUnary) {
        auto unaryNode = std::make_unique<semantic::UnaryOpNode>();
        annotate(unaryNode.get(), children(node).at(0));
        unaryNode->op = unary;
        unaryNode->operand = std::move(expr);
        expr = std::move(unaryNode);
    }

    for (index += 1; index + 1 < children(node).size(); index += 2) {
        auto binop = std::make_unique<semantic::BinOpNode>();
        annotate(binop.get(), children(node).at(index));
        binop->op = visit_AdditiveOperator(children(node).at(index));
        binop->lhs = std::move(expr);
        binop->rhs = visit_Term(children(node).at(index + 1));
        expr = std::move(binop);
    }
    return expr;
}

semantic::BinOpKind AstBuilder::visit_AdditiveOperator(const parse_tree::NodePtr& node) { 
    auto label = children(node).at(0)->getLabel();
    if (label == "plus") return semantic::BinOpKind::Add;
    if (label == "minus") return semantic::BinOpKind::Sub;
    return semantic::BinOpKind::Or;
}

semantic::AstPtr AstBuilder::visit_Term(const parse_tree::NodePtr& node) { 
    auto expr = visit_Factor(children(node).at(0));
    for (std::size_t i = 1; i + 1 < children(node).size(); i += 2) {
        auto binop = std::make_unique<semantic::BinOpNode>();
        annotate(binop.get(), children(node).at(i));
        binop->op = visit_MultiplicativeOperator(children(node).at(i));
        binop->lhs = std::move(expr);
        binop->rhs = visit_Factor(children(node).at(i + 1));
        expr = std::move(binop);
    }
    return expr;
}

semantic::BinOpKind AstBuilder::visit_MultiplicativeOperator(const parse_tree::NodePtr& node) { 
    auto label = children(node).at(0)->getLabel();
    if (label == "times") return semantic::BinOpKind::Mul;
    if (label == "idiv") return semantic::BinOpKind::IntDiv;
    if (label == "rdiv") return semantic::BinOpKind::Div;
    if (label == "imod") return semantic::BinOpKind::Mod;
    return semantic::BinOpKind::And;
}

semantic::AstPtr AstBuilder::visit_Factor(const parse_tree::NodePtr& node) { 
    const auto& first = children(node).at(0);
    if (isLabel(first, "lparent")) return visit_Expression(children(node).at(1));
    if (isLabel(first, "notsy")) {
        auto unary = std::make_unique<semantic::UnaryOpNode>();
        annotate(unary.get(), first);
        unary->op = semantic::UnaryOpKind::Not;
        unary->operand = visit_Factor(children(node).at(1));
        return unary;
    }
    if (isLabel(first, "procedure/function-call")) return visit_ProcedureFunctionCall(first);
    if (isLabel(first, "variable")) return visit_Variable(first);
    if (isLabel(first, "intcon") || isLabel(first, "realcon") || isLabel(first, "charcon") || isLabel(first, "string")) {
        return visit_Constant(first);
    }
    return nullptr;
}

semantic::AstPtr AstBuilder::visit_Variable(const parse_tree::NodePtr& node) { 
    semantic::AstPtr current = makeVar(children(node).at(0)->getValue(), children(node).at(0));
    for (std::size_t i = 1; i < children(node).size(); ++i) {
        const auto& selector = children(node).at(i);
        if (!isLabel(selector, "component-variable") && !isLabel(selector, "selector")) continue;
        if (isLabel(children(selector).at(0), "lbrack")) {
            auto access = std::make_unique<semantic::ArrayAccessNode>();
            annotate(access.get(), selector);
            access->base = std::move(current);
            access->indices = visit_IndexList(children(selector).at(1));
            current = std::move(access);
        } else {
            auto access = std::make_unique<semantic::FieldAccessNode>();
            annotate(access.get(), selector);
            access->base = std::move(current);
            access->field = children(selector).at(1)->getValue();
            current = std::move(access);
        }
    }
    return current;
}

semantic::AstPtr AstBuilder::visit_ComponentVariable(const parse_tree::NodePtr& node) { 
    return visit_Selector(node);
}

semantic::AstPtr AstBuilder::visit_Selector(const parse_tree::NodePtr& node) { 
    if (isLabel(children(node).at(0), "lbrack")) {
        auto access = std::make_unique<semantic::ArrayAccessNode>();
        annotate(access.get(), node);
        access->indices = visit_IndexList(children(node).at(1));
        return access;
    }
    auto access = std::make_unique<semantic::FieldAccessNode>();
    annotate(access.get(), node);
    access->field = children(node).at(1)->getValue();
    return access;
}

std::vector<semantic::AstPtr> AstBuilder::visit_IndexList(const parse_tree::NodePtr& node) { 
    std::vector<semantic::AstPtr> indices;
    for (const auto& child : children(node)) {
        if (isLabel(child, "expression")) indices.push_back(visit_Expression(child));
    }
    return indices;
}

semantic::AstPtr AstBuilder::visit_Constant(const parse_tree::NodePtr& node) { 
    if (isLabel(node, "constant")) {
        bool negate = false;
        std::size_t valueIndex = 0;
        if (isLabel(children(node).at(0), "plus") || isLabel(children(node).at(0), "minus")) {
            negate = isLabel(children(node).at(0), "minus");
            valueIndex = 1;
        }
        auto value = visit_Constant(children(node).at(valueIndex));
        if (!negate) return value;
        if (auto* intLit = dynamic_cast<semantic::IntLitNode*>(value.get())) {
            intLit->value = -intLit->value;
            return value;
        }
        if (auto* realLit = dynamic_cast<semantic::RealLitNode*>(value.get())) {
            realLit->value = -realLit->value;
            return value;
        }
        auto unary = std::make_unique<semantic::UnaryOpNode>();
        annotate(unary.get(), children(node).at(0));
        unary->op = semantic::UnaryOpKind::Minus;
        unary->operand = std::move(value);
        return unary;
    }

    auto label = node->getLabel();
    if (label == "intcon") {
        auto lit = std::make_unique<semantic::IntLitNode>();
        annotate(lit.get(), node);
        try { lit->value = std::stoll(node->getValue()); } catch (...) {}
        return lit;
    }
    if (label == "realcon") {
        auto lit = std::make_unique<semantic::RealLitNode>();
        annotate(lit.get(), node);
        try { lit->value = std::stod(node->getValue()); } catch (...) {}
        return lit;
    }
    if (label == "charcon") {
        auto lit = std::make_unique<semantic::CharLitNode>();
        annotate(lit.get(), node);
        try { lit->value = node->getValue().at(1); } catch (...) {}
        return lit;
    }
    if (label == "string") {
        auto lit = std::make_unique<semantic::StringLitNode>();
        annotate(lit.get(), node);
        try {
            std::string val = node->getValue().substr(1, node->getValue().size() - 2);
            std::size_t start = 0;
            while ((start = val.find("''", start)) != std::string::npos) {
                val.replace(start, 2, "'");
                ++start;
            }
            lit->value = val;
        } catch (...) {}
        return lit;
    }
    if (label == "ident" && (lower(node->getValue()) == "true" || lower(node->getValue()) == "false")) {
        auto lit = std::make_unique<semantic::BoolLitNode>();
        annotate(lit.get(), node);
        lit->value = lower(node->getValue()) == "true";
        return lit;
    }
    return makeVar(node->getValue(), node);
}

semantic::AstPtr AstBuilder::visit_Empty(const parse_tree::NodePtr& node) { 
    (void)node;
    return nullptr;
}
