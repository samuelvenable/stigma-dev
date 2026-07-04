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

#include "semantics.h"

#include "object_storage.h"

namespace enigma::parsing {

bool SemanticAnnotator::VisitBinaryExpression(AST::BinaryExpression &node) {
  if (node.operation.type == TT_DOT) classify_access(node);
  return true;
}

// Named dot accesses classify syntactically for now: global./local. are
// keywords in effect, and everything else routes through varaccess. The
// binder will add the MEMBER arm (lhs is a local declared with a
// definition-page struct type) and instance-id refinement.
void SemanticAnnotator::classify_access(AST::BinaryExpression &node) {
  using AccessKind = AST::BinaryExpression::AccessKind;
  if (node.left->type != AST::NodeType::IDENTIFIER ||
      node.right->type != AST::NodeType::IDENTIFIER) {
    return;  // Chained or computed lhs: stays UNRESOLVED until the binder.
  }
  const std::string &left = node.left->As<AST::IdentifierAccess>()->name.content;
  if (left == "global") {
    node.access_kind = AccessKind::GLOBAL;
  } else if (left == "local") {
    node.access_kind = AccessKind::LOCAL;
  } else {
    node.access_kind = AccessKind::VARACCESS;
  }
}

}  // namespace enigma::parsing

namespace {

void annotate(ParsedCode &code) {
  enigma::parsing::SemanticAnnotator annotator;
  code.ast.VisitNodes(annotator);
}

}  // namespace

void annotate_semantics(CompileState &state) {
  for (parsed_object *obj : state.parsed_objects)
    for (ParsedEvent &event : obj->all_events) annotate(event);
  for (ParsedScript *script : state.parsed_scripts) {
    annotate(script->code);
    if (script->global_code) annotate(*script->global_code);
  }
  for (ParsedScript *tline : state.parsed_tlines) {
    annotate(tline->code);
    if (tline->global_code) annotate(*tline->global_code);
  }
  for (parsed_room *room : state.parsed_rooms) {
    if (room->creation_code) annotate(*room->creation_code);
    for (auto &[id, icc] : room->instance_create_codes)
      if (icc.code) annotate(*icc.code);
    for (auto &[id, icc] : room->instance_precreate_codes)
      if (icc.code) annotate(*icc.code);
  }
}
