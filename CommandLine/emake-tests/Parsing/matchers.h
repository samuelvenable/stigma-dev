/** Copyright (C) 2024 Fares Atef
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

#ifndef MATCHERS_H
#define MATCHERS_H

#include <gmock/gmock.h>
using namespace ::enigma::parsing;
using namespace ::testing;

extern std::string ExpectedMsg;

// Crawl a declarator-expression-tree into its JDI-bridge ref_stack and report
// whether it carries no `* & [] ()` modifiers -- i.e. a plain/abstract base
// type. Shared replacement for the old Declarator::components.size()==0 /
// has_nested_declarator bridge assertions.
inline bool declarator_is_unqualified(AST::Node *declarator_expr) {
  jdi::ref_stack rs;
  return walk_declarator_expr(declarator_expr, rs) && rs.empty();
}

// Same check over every declarator in a clause (e.g. a cast/type-id whose
// abstract declarator should be modifier-free).
inline bool clause_is_unqualified(AST::DeclaratorClause *clause) {
  for (auto &id : clause->declarators)
    if (!declarator_is_unqualified(id->declarator_expr.get())) return false;
  return true;
}

// Whether every declarator in a clause is unnamed (abstract): the declarator-id
// name is empty. Distinct from clause_is_unqualified (which checks the `* & [] ()`
// modifiers): an abstract type-id is both unqualified AND unnamed. Replaces the
// old `FullType::decl.name.content == ""` bridge assertion -- the name now lives
// on each InitDeclarator, not on a Declarator embedded in the FullType.
inline bool clause_is_unnamed(AST::DeclaratorClause *clause) {
  for (auto &id : clause->declarators)
    if (!id->name.content.empty()) return false;
  return true;
}

MATCHER_P2(IsDeclaration, decls, decl_type, "") {
  if (arg->type != AST::NodeType::DECLARATION) {
    ExpectedMsg = "From IsDeclaration Matcher: NodeType = DECLARATION\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *decl = arg->template As<AST::DeclarationStatement>();
  if (!decl) {
    ExpectedMsg += "From IsDeclaration Matcher: decl isn't nullptr\n";
    *result_listener << "got decl = nullptr\n";
    return false;
  }

  bool b1 = decl->storage_class == AST::DeclarationStatement::StorageClass::TEMPORARY,
       b2 = decl->clause->declarators.size() == decls.size();

  if (!b1 || !b2) {
    ExpectedMsg = "From IsDeclaration Matcher: ";

    if (!b1) {
      ExpectedMsg += "StorageClass = TEMPORARY\n";
      *result_listener << "got StorageClass = " << AST::DeclarationStatement::StorageToString(decl->storage_class)
                       << "\n";
    }
    if (!b2) {
      ExpectedMsg += "DeclarationsSize = " + to_string(decls.size()) + "\n";
      *result_listener << "got DeclarationsSize = " << to_string(decl->clause->declarators.size()) << "\n";
    }
  }

  bool b3 = true;
  auto *expected_def = static_cast<jdi::definition*>(decl_type);
  // Base type + cv/sign/length flags live on the shared spec-seq now, not a
  // per-declarator FullType: `specifiers->Definition()` is the resolved base
  // type (read from the id-expression tree) and `declspecs->flags` is the same
  // bitmask the deleted `declarator->flags` carried. The per-name declarator
  // modifiers are on `declarator_expr`.
  auto *spec = decl->clause->specifiers.get();
  auto *parsed_def = spec->Definition();
  for (size_t i = 0; i < decls.size(); i++) {
    auto &init_decl = *decl->clause->declarators[i];
    b3 = b3 && init_decl.init != nullptr;
    // Type comparison: if expected_def is nullptr, skip the check (any type is OK)
    // Otherwise, compare pointers OR compare by name for builtin types
    bool type_matches = (expected_def == nullptr) ||
                        (parsed_def == expected_def) ||
                        (parsed_def && expected_def && parsed_def->name == expected_def->name);
    b3 = b3 && type_matches;
    b3 = b3 && (!spec->declspecs || spec->declspecs->flags == 0);
    b3 = b3 && init_decl.name.content == decls[i];
    // These matcher uses are all plain-name declarators (`int x, y;`): the
    // declarator-expression-tree carries no pointer/array/function modifiers.
    b3 = b3 && declarator_is_unqualified(init_decl.declarator_expr.get());
    if (!b3) {
      if (ExpectedMsg == "") ExpectedMsg = "From IsDeclaration Matcher: ";
      std::string expected_type = expected_def ? expected_def->name : "any";
      std::string got_type = parsed_def ? parsed_def->name : "nullptr";
      ExpectedMsg +=
          "Declaration [" + to_string(i) +
          "] has init != nullptr, def = " + expected_type + ", flags = 0, name.content = " +
          decls[i] + ", empty ref_stack\n";
      *result_listener << " got Declaration [" << to_string(i) << "] has init "
                       << ((init_decl.init) ? "!=" : "=")
                       << " nullptr, def = " << got_type << ", flags = "
                       << to_string(spec->declspecs ? spec->declspecs->flags : 0)
                       << ", name.content = " << init_decl.name.content
                       << ", unqualified = " << to_string(declarator_is_unqualified(init_decl.declarator_expr.get()))
                       << "\n";
    }
  }

  return b1 && b2 && b3;
}

MATCHER_P3(IsCast, cast_kind, expr_type, type, "") {
  if (arg->type != AST::NodeType::CAST) {
    ExpectedMsg = "From IsCast Matcher: NodeType = CAST\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *cast = arg->template As<AST::CastExpression>();
  if (!cast) {
    ExpectedMsg += "From IsCast Matcher: cast isn't nullptr\n";
    *result_listener << "got cast = nullptr\n";
    return false;
  }

  auto *expr = cast->expr->template As<AST::Node>();
  if (!expr) {
    ExpectedMsg += "From IsCast Matcher: expr isn't nullptr\n";
    *result_listener << "got expr = nullptr\n";
    return false;
  }

  // cast->type is a PNode wrapping a DeclaratorClause; the cast's type is its
  // `specifiers` (a TypeSpecifierSeq), whose `Definition()`/`flags` are the
  // resolved base type and cv/sign/length bitmask. The clause's abstract declarator (the
  // `*`/`&`/`[]` chain) is checked for emptiness via clause_is_unqualified /
  // clause_is_unnamed below, since these are simple-type cast tests.
  auto *clause = cast->type ? cast->type->template As<AST::DeclaratorClause>() : nullptr;
  auto *typeid_node = clause ? clause->specifiers.get() : nullptr;
  if (!typeid_node) {
    ExpectedMsg += "From IsCast Matcher: cast->type is a DeclaratorClause with specifiers\n";
    *result_listener << "got cast->type = "
                     << (cast->type ? AST::NodeToString(cast->type->type) : std::string{"nullptr"}) << "\n";
    return false;
  }
  auto &ft = *typeid_node;
  // Type comparison: if expected_def is nullptr, skip the check (any type is OK)
  // Otherwise, compare pointers OR compare by name for builtin types
  auto *parsed_def = ft.Definition();
  auto *expected_def = static_cast<jdi::definition*>(type);
  bool b1 = (expected_def == nullptr) || (parsed_def == expected_def) ||
            (parsed_def && expected_def && parsed_def->name == expected_def->name);
  // These casts are all simple-type (`(int)x`, `static_cast<int>(...)`): the
  // abstract declarator carries no `* & [] ()` modifiers (b3) and no name (b4).
  bool b2 = ft.flags == 0, b3 = clause_is_unqualified(clause), b4 = clause_is_unnamed(clause),
       b6 = cast->kind == cast_kind, b7 = expr->type == expr_type;

  bool res = b1 && b2 && b3 && b4 && b6 && b7;

  if (!res) {
    ExpectedMsg = "From IsCast Matcher: ";

    if (!b2) {
      ExpectedMsg += "ft.flags = 0\n";
      *result_listener << "got ft.flags = " << to_string(ft.flags) << "\n";
    }
    if (!b3) {
      ExpectedMsg += "cast type-id has no declarator modifiers (empty ref_stack)\n";
      *result_listener << "got a non-empty declarator ref_stack on the cast type-id\n";
    }
    if (!b4) {
      ExpectedMsg += "cast type-id is unnamed (abstract declarator)\n";
      *result_listener << "got a named declarator on the cast type-id\n";
    }
    if (!b6) {
      ExpectedMsg += "cast->kind = " + AST::CastExpression::KindToString(cast_kind) + "\n";
      *result_listener << "got cast->kind = " << AST::CastExpression::KindToString(cast->kind) << "\n";
    }
    if (!b7) {
      ExpectedMsg += "expr->type = " + AST::NodeToString(expr_type) + "\n";
      *result_listener << "got expr->type = " << AST::NodeToString(expr->type) << "\n";
    }
  }

  return res;
}

MATCHER_P(IsIdentifier, iden, "") {
  if (arg->type != AST::NodeType::IDENTIFIER) {
    ExpectedMsg += "From IsIdentifier Matcher: NodeType = IDENTIFIER\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *iden_access = arg->template As<AST::IdentifierAccess>();
  if (!iden_access) {
    ExpectedMsg += "From IsIdentifier Matcher: iden_access isn't nullptr\n";
    *result_listener << "got iden_access = nullptr\n";
    return false;
  }

  bool b1 = iden_access->name.content == iden;
  if (!b1) {
    ExpectedMsg += "From IsIdentifier Matcher: ";
    ExpectedMsg += "Identifier = " + PrintToString(iden) + "\n";
    *result_listener << "got Identifier = " << arg->template As<AST::IdentifierAccess>()->name.content << "\n";
    return false;
  }

  return true;
}

MATCHER_P(IsLiteral, lit, "") {
  if (arg->type != AST::NodeType::LITERAL) {
    ExpectedMsg += "From IsLiteral Matcher: NodeType = LITERAL\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *literal = arg->template As<AST::Literal>();
  if (!literal) {
    ExpectedMsg += "From IsLiteral Matcher: literal isn't nullptr\n";
    *result_listener << "got literal = nullptr\n";
    return false;
  }

  bool b1 = std::get<std::string>(literal->value.value) == lit;
  if (!b1) {
    ExpectedMsg += "From IsLiteral Matcher: ";
    ExpectedMsg += "Literal = " + PrintToString(lit) + "\n";
    *result_listener << "got Literal = " << std::get<std::string>(arg->template As<AST::Literal>()->value.value)
                     << "\n";
    return false;
  }

  return true;
}

MATCHER_P3(IsBinaryOperation, op, M1, M2, "") {
  if (arg->type != AST::NodeType::BINARY_EXPRESSION) {
    ExpectedMsg += "From IsBinaryOperation Matcher: NodeType = BINARY_EXPRESSION\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *binary = arg->template As<AST::BinaryExpression>();
  if (!binary) {
    ExpectedMsg += "From IsBinaryOperation Matcher: binary isn't nullptr\n";
    *result_listener << "got binary = nullptr\n";
    return false;
  }

  bool b1 = binary->operation.type == op, b2 = ExplainMatchResult(M1, binary->left, result_listener),
       b3 = ExplainMatchResult(M2, binary->right, result_listener);

  bool res = b1 && b2 && b3;
  if (!b1) {
    ExpectedMsg += "From IsBinaryOperation Matcher: ";
    ExpectedMsg += "Operation = " + ToString(op) + "\n";
    *result_listener << "got Operation = " << ToString(binary->operation.type) << "\n";
  }

  return res;
}

MATCHER_P2(IsUnaryPostfixOperator, op, M1, "") {
  if (arg->type != AST::NodeType::UNARY_POSTFIX_EXPRESSION) {
    ExpectedMsg = "From IsUnaryPostfixOperator Matcher: NodeType = UNARY_POSTFIX_EXPRESSION\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *unary = arg->template As<AST::UnaryPostfixExpression>();
  if (!unary) {
    ExpectedMsg += "From IsUnaryPostfixOperator Matcher: unary isn't nullptr\n";
    *result_listener << "got unary = nullptr\n";
    return false;
  }

  bool b1 = unary->operation.type == op, b2 = ExplainMatchResult(M1, unary->operand, result_listener);

  bool res = b1 && b2;
  if (!b1) {
    ExpectedMsg = "From IsUnaryPostfixOperator Matcher: ";
    ExpectedMsg += "Operation = " + ToString(op) + "\n";
    *result_listener << "got Operation = " << ToString(unary->operation.type) << "\n";
  }

  return res;
}

MATCHER_P2(IsUnaryPrefixOperator, op, M1, "") {
  if (arg->type != AST::NodeType::UNARY_PREFIX_EXPRESSION) {
    ExpectedMsg = "From IsUnaryPrefixOperator Matcher: NodeType = UNARY_PREFIX_EXPRESSION\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *unary = arg->template As<AST::UnaryPrefixExpression>();
  if (!unary) {
    ExpectedMsg += "From IsUnaryPrefixOperator Matcher: unary isn't nullptr\n";
    *result_listener << "got unary = nullptr\n";
    return false;
  }

  bool b1 = unary->operation.type == op, b2 = ExplainMatchResult(M1, unary->operand, result_listener);

  bool res = b1 && b2;
  if (!b1) {
    ExpectedMsg = "From IsUnaryPrefixOperator Matcher: ";
    ExpectedMsg += "Operation = " + ToString(op) + "\n";
    *result_listener << "got Operation = " << ToString(unary->operation.type) << "\n";
  }

  return res;
}

MATCHER_P(IsParenthetical, M1, "") {
  if (arg->type != AST::NodeType::PARENTHETICAL) {
    ExpectedMsg = "From IsParenthetical Matcher: NodeType = PARENTHETICAL\n";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *paren = arg->template As<AST::Parenthetical>();
  if (!paren) {
    ExpectedMsg += "From IsParenthetical Matcher: paren isn't nullptr\n";
    *result_listener << "got paren = nullptr\n";
    return false;
  }

  return ExplainMatchResult(M1, paren->expression, result_listener);
}

MATCHER_P(IsStatementBlock, stateSize, "") {
  if (arg->type != AST::NodeType::BLOCK) {
    ExpectedMsg = "From IsStatementBlock Matcher: NodeType = BLOCK";
    *result_listener << "got NodeType = " << AST::NodeToString(arg->type) << "\n";
    return false;
  }

  auto *block = arg->template As<AST::CodeBlock>();
  if (!block) {
    ExpectedMsg += "From IsStatementBlock Matcher: block isn't nullptr\n";
    *result_listener << "got block = nullptr\n";
    return false;
  }

  bool b1 = block->statements.size() == size_t(stateSize);

  if (!b1) {
    ExpectedMsg = "From IsStatementBlock Matcher: ";
    ExpectedMsg = "IsStatementBlock Matcher: Statements Size = " + to_string(stateSize);
    *result_listener << "Statements Size = " << to_string(block->statements.size()) << "\n";
    return false;
  }

  return true;
}

MATCHER_P4(IsForLoopWithChildren, M1, M2, M3, M4, ExpectedMsg) {
  auto *assign = arg->assignment.get();
  auto *binary = arg->condition.get();
  auto *unary = arg->increment.get();
  auto *block = arg->body.get();

  return ExplainMatchResult(M1, assign, result_listener) && ExplainMatchResult(M2, binary, result_listener) &&
         ExplainMatchResult(M3, unary, result_listener) && ExplainMatchResult(M4, block, result_listener);
}

#endif
