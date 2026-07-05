/* Copyright (C) 2026 Josh Ventura
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
*/

#ifndef ENIGMA_SEMANTICS_H
#define ENIGMA_SEMANTICS_H

#include "parsing/ast.h"

#include <string>
#include <unordered_map>

struct CompileState;

namespace enigma::parsing {

/// Post-link semantic annotation. Walks each parsed AST once, after the
/// linking phase has harvested locals across all objects, and writes
/// semantic facts onto the nodes for the printers (and future checks) to
/// read. Each concern is a named helper dispatched from the node visits;
/// canonicalization lives here, never in the parser.
class SemanticAnnotator : public AST::Visitor {
 public:
  SemanticAnnotator(ErrorHandler *herr, const LanguageFrontend *frontend)
      : herr_(herr), frontend_(frontend) {}

  bool VisitScopeAccess(AST::ScopeAccess &node) final;
  bool VisitFunctionCallExpression(AST::FunctionCallExpression &node) final;
  bool VisitDeclarationStatement(AST::DeclarationStatement &node) final;

  /// Register a name as a class-typed local ahead of the walk. The walk
  /// itself records declarations it encounters; this is the seam for scopes
  /// harvested elsewhere (and for tests).
  void DeclareLocal(std::string_view name, jdi::definition *def) {
    struct_locals_[std::string(name)] = def;
  }

 private:
  void classify_access(AST::ScopeAccess &node);
  void validate_call(AST::FunctionCallExpression &node);
  void record_locals(AST::DeclarationStatement &node);

  ErrorHandler *herr_;
  const LanguageFrontend *frontend_;
  // Locals declared with a class type (definition pages) in this AST, in
  // walk order. Flat per-event scoping for now; block scopes can refine it.
  std::unordered_map<std::string, jdi::definition*> struct_locals_;
};

}  // namespace enigma::parsing

/// Annotate every parsed AST in the game: object events, scripts,
/// timelines, and room creation code.
void annotate_semantics(CompileState &state);

#endif
