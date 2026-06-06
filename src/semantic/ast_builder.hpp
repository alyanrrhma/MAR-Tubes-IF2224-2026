#pragma once

#include "../parser/parse_tree.hpp"
#include "ast_nodes.hpp"

#include <tuple>
#include <utility>

class AstBuilder
{
private:
    std::unique_ptr<semantic::ProgramNode> visit_Program(const parse_tree::NodePtr& node); 
    std::string visit_ProgramHeader(const parse_tree::NodePtr& node); 
    std::string visit_ProgramHeading(const parse_tree::NodePtr& node); 
    std::vector<std::string> visit_IdentifierList(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::BlockNode> visit_Block(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::DeclarationNode> visit_DeclarationPart(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_ConstDeclaration(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_ConstantDeclaration(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ConstDeclNode> visit_ConstDefinition(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ConstDeclNode> visit_ConstantDefinition(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_TypeDeclaration(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::TypeDeclNode> visit_TypeDefinition(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_VarDeclaration(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_VariableDeclaration(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Type(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_TypeDenoter(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ArrayTypeNode> visit_ArrayType(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::RangeNode> visit_Range(const parse_tree::NodePtr& node); 
    std::vector<std::string> visit_Enumerated(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::RecordTypeNode> visit_RecordType(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::RecordTypeNode> visit_FieldList(const parse_tree::NodePtr& node); 
    semantic::RecordTypeNode::FieldSection visit_FieldPart(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_SubprogramDeclarationPart(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_SubprogramDeclaration(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ProcDeclNode> visit_ProcedureDeclaration(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::FuncDeclNode> visit_FunctionDeclaration(const parse_tree::NodePtr& node); 
    std::pair<std::string, std::vector<semantic::FormalParam>> visit_ProcedureHeading(const parse_tree::NodePtr& node); 
    std::tuple<std::string, std::vector<semantic::FormalParam>, std::string> visit_FunctionHeading(const parse_tree::NodePtr& node); 
    std::vector<semantic::FormalParam> visit_FormalParameterList(const parse_tree::NodePtr& node); 
    semantic::FormalParam visit_ParameterGroup(const parse_tree::NodePtr& node); 
    semantic::FormalParam visit_FormalParameterSection(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::CompoundNode> visit_CompoundStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::CompoundNode> visit_StatementList(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Statement(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_AssignmentStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ProcCallNode> visit_ProcedureFunctionCall(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ProcCallNode> visit_ProcedureCallStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::IfNode> visit_IfStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::CaseNode> visit_CaseStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::CaseBranchNode> visit_CaseBlock(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::RepeatNode> visit_RepeatStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::WhileNode> visit_WhileStatement(const parse_tree::NodePtr& node); 
    std::unique_ptr<semantic::ForNode> visit_ForStatement(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_ParameterList(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Expression(const parse_tree::NodePtr& node); 
    semantic::BinOpKind visit_RelationalOperator(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_SimpleExpression(const parse_tree::NodePtr& node); 
    semantic::BinOpKind visit_AdditiveOperator(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Term(const parse_tree::NodePtr& node); 
    semantic::BinOpKind visit_MultiplicativeOperator(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Factor(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Variable(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_ComponentVariable(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Selector(const parse_tree::NodePtr& node); 
    std::vector<semantic::AstPtr> visit_IndexList(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Constant(const parse_tree::NodePtr& node); 
    semantic::AstPtr visit_Empty(const parse_tree::NodePtr& node); 
private:
    std::string currentFunctionName_;

public:
    semantic::AstPtr build(const parse_tree::NodePtr& root); 
};
