/** Copyright (C) 2022 Dhruv Chawla
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
#ifndef ENIGMA_COMPILER_PARSING_DECLARATOR_h
#define ENIGMA_COMPILER_PARSING_DECLARATOR_h

#include <JDI/src/Storage/definition.h>

#include "tokens.h"

/* Thin name-holder produced by the id-expression parser. The declarator's
 * structure (pointers, arrays, function parameters, nesting) now lives in the
 * AST declarator-expression-tree; the JDI ref_stack is built from it by
 * walk_declarator_expr. This struct only carries the declared name and the
 * definition its id-expression resolved to. */
namespace enigma::parsing {
struct Declarator {
  /// The name of the variable being declared (the @c x in <tt> **(*x)[10] </tt>).
  Token name;

  /// The definition that this declarator's id-expression resolved to.
  jdi::definition *ndef = nullptr;

  Declarator() noexcept = default;
  Declarator(Declarator &&) noexcept = default;
  Declarator &operator=(Declarator &&) noexcept = default;
  Declarator(const Declarator &) = delete;
  Declarator &operator=(const Declarator &) = delete;
};

}

#endif
