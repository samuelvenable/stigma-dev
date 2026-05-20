/** Copyright (C) 2020 Josh Ventura
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "ast.h"
#include "parser.h"
#include "parser/collect_variables.h"  // collect_variables
#include "macro_helper.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace enigma::parsing {

bool AST::empty() const { return !root_; }

// Utility routine: Apply this AST to a specified instance.
void AST::ApplyTo(int instance_id) {
  apply_to_ = instance_id;
}

void AST::WriteCppToStream(std::ofstream &of, int base_indent, bool is_script) const {
  CppPrettyPrinter visitor(of, lexer->GetContext().language_fe, is_script);
  if (apply_to_) {
    of << std::string(base_indent, ' ') << "with (" << *apply_to_ << ") {\n";
  }

  if (root_) root_->accept(visitor);
  
  if (apply_to_) {
    of << std::string(base_indent, ' ') << "}\n";
  }
}

AST AST::Parse(std::string_view code, const ParseContext* ctex) {
  AST res(code, ctex);
  res.root_ = enigma::parsing::Parse(res.lexer.get(), &res.herr);
  return res;
}

void AST::ExtractDeclarations(ParsedScope *destination_scope, CompileState *cs) {
  std::cout << "collecting variables..." << std::flush;
  const ParseContext &ctex = lexer->GetContext();
  collect_variables(ctex.language_fe, this, destination_scope, ctex.script_names, cs);
  std::cout << " done." << std::endl;
}

template<typename... SubNodes>
void AST::Node::RV(AST::Visitor &visitor, const SubNodes &...nodes) {
  (RVF(visitor, nodes), ...);
}

void AST::CodeBlock::RecursiveSubVisit(AST::Visitor &visitor) {
  RV(visitor, statements);
}
void AST::BinaryExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, left, right);
}
void AST::FunctionCallExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, function, arguments);
}
void AST::UnaryPrefixExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, operand);
}
bool AST::UnaryPrefixExpression::CanBeTypeSpecifier() const {
  // C++ pointer/reference declarator operators: *, &, &&.
  return operation.type == TT_STAR
      || operation.type == TT_AMPERSAND
      || operation.type == TT_AND;
}
void AST::UnaryPostfixExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, operand);
}
void AST::TernaryExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, condition, true_expression, false_expression);
}
void AST::TypeId::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, id_expression, declspecs);
}
jdi::full_type AST::TypeId::to_jdi_fulltype() {
  // Routes through the cached FullType (the JDI-bridge layer's form of this
  // type). Today type_info is populated at construction by the spec-seq +
  // declarator parse; post-4e it gets lazily synthesized from declspecs +
  // declarator-expression-tree on first call. Returned by value because
  // downstream callers `swap_in` the result into JDI structures.
  return type_info.to_jdi_fulltype();
}
void AST::DeclSpecList::RecursiveSubVisit(Visitor &visitor) {
  (void) visitor;  // Leaf: specs are Tokens, not AST nodes.
}
void AST::LambdaExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, parameters, body);
}
void AST::SizeofExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, argument);
}
void AST::AlignofExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, type);
}
void AST::CastExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, expr);
}
void AST::Parenthetical::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, expression);
}
void AST::Array::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, elements);
}
void AST::IdentifierAccess::RecursiveSubVisit(Visitor &visitor) {
  (void) visitor;  // Varname token that appeared in code; it's a leaf.
}
void AST::Literal::RecursiveSubVisit(Visitor &visitor) {
  (void) visitor;  // Literal in the middle of code; also a leaf.
}
void AST::IfStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, condition, true_branch, false_branch);
}
void AST::ForLoop::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, assignment, condition, increment, body);
}
void AST::WhileLoop::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, condition, body);
}
void AST::DoLoop::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, body, condition);
}
void AST::CaseStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, value);
}
void AST::DefaultStatement::RecursiveSubVisit(Visitor &visitor) {
  (void) visitor;  // Label must preceed a statement, but does not own it
}
void AST::SwitchStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, expression);
}
void AST::ReturnStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, expression);
}
void AST::BreakStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, count);
}
void AST::ContinueStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, count);
}
void AST::WithStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, object, body);
}
void AST::Initializer::RecursiveSubVisit(Visitor &visitor) {
  if (target) RV(visitor, target);
  RV(visitor, values);
}
void AST::NewExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, placement_args, type, initializer);
}
void AST::DeleteExpression::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, expression);
}
void AST::DeclarationStatement::RecursiveSubVisit(Visitor &visitor) {
  RV(visitor, type);
  // TRANSITIONAL: this loop disappears in phase 2 step 4 when Declaration
  // becomes an AST::Node and declarations becomes vector<unique_ptr<Declaration>>.
  // Then the line is just: RV(visitor, declarations);
  for (auto &decl : declarations) RV(visitor, decl.init);
}

const std::vector<std::string> AST::NodesNames = ENUM_NAME_VECTOR(NodeType,
    ERROR,
    BLOCK,
    BINARY_EXPRESSION,
    UNARY_PREFIX_EXPRESSION,
    UNARY_POSTFIX_EXPRESSION,
    TERNARY_EXPRESSION,
    LAMBDA_EXPRESSION,
    TYPE_ID,
    DECL_SPEC_LIST,
    SIZEOF,
    ALIGNOF,
    CAST,
    NEW,
    DELETE,
    PARENTHETICAL,
    ARRAY,
    IDENTIFIER,
    SCOPE_ACCESS,
    LITERAL,
    FUNCTION_CALL,
    IF,
    FOR,
    WHILE,
    DO,
    WITH,
    REPEAT,
    SWITCH,
    CASE,
    DEFAULT,
    BREAK,
    CONTINUE,
    RETURN,
    DECLARATION,
    INITIALIZER);

const std::vector<std::string> AST::DeclarationStatement::StorageNames =
    ENUM_NAME_VECTOR(StorageClass, TEMPORARY, LOCAL, GLOBAL);

const std::vector<std::string> AST::CastExpression::KindNames = ENUM_NAME_VECTOR(Kind,
    C_STYLE,
    STATIC,
    DYNAMIC,
    REINTERPRET,
    CONST);

std::string AST::NodeToString(NodeType nt){
  return NodesNames[(int)nt];
}

std::string AST::DeclarationStatement::StorageToString(StorageClass st){
  return StorageNames[(int)st];
}

std::string AST::CastExpression::KindToString(Kind k){
  return KindNames[(int)k];
}

}  // namespace enigma::parsing
