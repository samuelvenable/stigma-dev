/**
 * @file clang_definitions.h
 * @brief Definition structures that mirror JDI interface but use clang
 * 
 * This file provides compatibility structures that match JDI's definition
 * interface but are backed by clang AST nodes.
 */

#ifndef ENIGMA_CLANG_DEFINITIONS_H
#define ENIGMA_CLANG_DEFINITIONS_H

#include <clang-c/Index.h>
#include <string>
#include <map>
#include <memory>
#include <vector>

// T0: this branch still compiles the real JDI, so DEF_FLAGS comes from
// there instead of a re-declared compatibility enum (values verified
// identical against jdi::DEF_FLAGS in Storage/definition_forward.h).
#include <Storage/definition.h>

namespace clang_adapter {

// Base definition class that mirrors JDI's definition structure
struct ClangDefinition {
  unsigned int flags;
  std::string name;
  struct ClangDefinitionScope* parent;
  CXCursor cursor;  // The clang cursor this represents
  
  ClangDefinition(const std::string& n, struct ClangDefinitionScope* p, unsigned int f, CXCursor c)
    : flags(f), name(n), parent(p), cursor(c) {}
  
  virtual ~ClangDefinition() = default;
  
  bool has_all_flags(unsigned flgs) const { return (flags & flgs) == flgs; }
  
  std::string qualified_id() const;
  std::string toString() const;
};

// Forward declaration
struct ClangDefinitionScope;

// Forward declaration for shared_ptr
struct ClangDefinitionScope;

// Scope definition (namespace, class, struct, union)
struct ClangDefinitionScope : public ClangDefinition {
  std::map<std::string, std::shared_ptr<ClangDefinition>> members;
  
  ClangDefinitionScope(const std::string& n, ClangDefinitionScope* p, unsigned int f, CXCursor c)
    : ClangDefinition(n, p, f, c) {}
  
  ClangDefinition* look_up(const std::string& name);
  ClangDefinition* find_local(const std::string& name);
};

// Class definition
struct ClangDefinitionClass : public ClangDefinitionScope {
  std::vector<std::pair<ClangDefinitionClass*, int>> ancestors;  // base classes
  
  ClangDefinitionClass(const std::string& n, ClangDefinitionScope* p, unsigned int f, CXCursor c)
    : ClangDefinitionScope(n, p, f, c) {}
};

// Function overload
struct ClangDefinitionOverload : public ClangDefinition {
  // Parameters stored as type definitions
  std::vector<ClangDefinition*> params;
  // Storage for parameter shared_ptrs to keep them alive
  std::vector<std::shared_ptr<ClangDefinition>> owned_params_storage;
  bool is_variadic;
  
  ClangDefinitionOverload(const std::string& n, ClangDefinitionScope* p, unsigned int f, CXCursor c)
    : ClangDefinition(n, p, f, c), is_variadic(false) {}
  
  // Delete copy constructor and assignment to prevent invalidating raw pointers in params
  // The raw pointers in params depend on owned_params_storage, and copying would break this relationship
  ClangDefinitionOverload(const ClangDefinitionOverload&) = delete;
  ClangDefinitionOverload& operator=(const ClangDefinitionOverload&) = delete;
  
  // Allow move constructor - when moved, both vectors move together, so raw pointers remain valid
  ClangDefinitionOverload(ClangDefinitionOverload&&) = default;
  ClangDefinitionOverload& operator=(ClangDefinitionOverload&&) = default;
};

// Function definition (contains overloads)
struct ClangDefinitionFunction : public ClangDefinition {
  std::map<std::string, std::shared_ptr<ClangDefinitionOverload>> overloads;
  std::vector<std::shared_ptr<ClangDefinitionOverload>> template_overloads;
  
  ClangDefinitionFunction(const std::string& n, ClangDefinitionScope* p, unsigned int f, CXCursor c)
    : ClangDefinition(n, p, f, c) {}
};

// Typed definition (variables, typedefs)
struct ClangDefinitionTyped : public ClangDefinition {
  ClangDefinition* type;
  
  ClangDefinitionTyped(const std::string& n, ClangDefinitionScope* p, unsigned int f, CXCursor c, ClangDefinition* t)
    : ClangDefinition(n, p, f, c), type(t) {}
};

// Helper function to convert clang cursor kind to JDI flags
unsigned int cursor_kind_to_flags(CXCursorKind kind);

// Helper function to get cursor name
std::string get_cursor_name(CXCursor cursor);

} // namespace clang_adapter

// T0: no jdi:: compatibility typedefs here -- the real jdi::definition,
// jdi::definition_scope, etc. (and the real builtin_flag__*/builtin_type__int
// globals in System/builtins.h) already exist on this branch and would
// collide. clang_adapter's own types stay in clang_adapter:: until T1
// retargets population onto jdi::Storage per JDI2-DESIGN.md.

#endif // ENIGMA_CLANG_DEFINITIONS_H
