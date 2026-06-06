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
#ifndef ENIGMA_COMPILER_PARSING_FULL_TYPE_h
#define ENIGMA_COMPILER_PARSING_FULL_TYPE_h

#include <cstddef>

#include <JDI/src/Storage/definition.h>

namespace enigma::parsing {
// Parser-local scratch for a base type: a resolved JDI definition plus the
// decl-spec flag bitmask (cv/sign/length, in JDI's encoding). The declarator
// structure (pointers, arrays, params, nesting) is NOT here -- it lives in the
// AST declarator-expression-tree and is bridged to JDI by walk_declarator_expr.
// FullType is no longer stored on the AST: the spec-parsing out-param functions
// fill one of these, and the owning TypeSpecifierSeq mirrors out its def+flags.
struct FullType {
  jdi::definition *def = nullptr;
  std::size_t flags = 0;

  FullType() noexcept = default;
  FullType(FullType &&) noexcept = default;
  FullType &operator=(FullType &&) noexcept = default;
};
}

#endif
