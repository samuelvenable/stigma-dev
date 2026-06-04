#ifndef ENIGMA_COMPILER_PARSING_PARSER_h
#define ENIGMA_COMPILER_PARSING_PARSER_h

#include "ast.h"
#include "full_type.h"
#include "language_frontend.h"
#include "lexer.h"
#include "precedence.h"
#include "settings.h"
#include "tokens.h"

#include <JDI/src/System/builtins.h>
#include <memory>

namespace enigma::parsing {

class AstBuilderTestAPI {
 public:
  using SyntaxMode = setting::SyntaxMode;

  Lexer *lexer;
  ErrorHandler *herr;
  SyntaxMode mode;
  const LanguageFrontend *frontend;
  Token token;

  /**
 * @brief Store a mapping from variable name to the @c FullType of its definition
 *
 * This is designed around the assumption that EDL does not yet support namespaces, so there is no need to consider
 * stacks here. If EDL were to support namespaces, this would have to be changed to a <tt> std::stack<...> </tt> and the
 * namespace or nested scope parser would have to push a new map onto the stack.
 */
  std::unordered_map<std::string, FullType *> declarations;

  // When true, TryParseOperand yields an empty-name placeholder leaf instead
  // of erroring on a missing operand -- i.e. we are inside a type-id /
  // declarator production where an abstract (nameless) declarator is
  // grammatical (cast targets, sizeof/alignof/new operands, function-params,
  // template type-args, declarator bodies). Set via ScopedFlag at those
  // boundaries, and flipped back to false when descending into embedded
  // value-expressions (array bounds, default arguments). Defaults false so
  // pure-expression contexts keep parse-time "missing operand" errors.
  bool allow_abstract_operand_ = false;

  // When true, a type-name operand is promoted to a full type-id clause
  // (specifiers + declarator) instead of the bare TypeSpecifierSeq that the
  // surrounding ParseExpression early-returns on. Set across a parenthesized
  // group that *might* be a declarator list (we don't yet know -- the role is
  // deferred to the semantic phase: it could be a cast, tie, tuple, lambda
  // params, or plain grouping), so ParseExpression itself can build a comma-
  // list of (possibly abstract) declarators. Inert until a type-name is
  // actually reached, so non-type parens are unaffected. Cousin of
  // allow_abstract_operand_; flips back to false at the same value-expression
  // boundaries (array bounds, default-init). Defaults false so a bare type-id
  // at statement level still bails to the caller for decl-vs-expr resolution.
  bool maybe_declarator_group_ = false;

  AstBuilderTestAPI() = default;

  void initialize(Lexer *lexer, ErrorHandler *herr) {
    this->lexer = lexer;
    this->herr = herr;
    this->mode = lexer->GetContext().compatibility_opts.syntax_mode;
    this->frontend = lexer->GetContext().language_fe;
    token = lexer->ReadToken();
  }

  virtual std::unique_ptr<AST::Node> TryParseStatement() = 0;
  virtual std::unique_ptr<AST::CodeBlock> ParseCode() = 0;
  virtual const Token &current_token() = 0;
  virtual std::unique_ptr<AST::CodeBlock> ParseCodeBlock() = 0;
  virtual std::unique_ptr<AST::TypeSpecifierSeq> TryParseTypeID() = 0;
  virtual std::unique_ptr<AST::DeclaratorClause> ParseTypeIdClause() = 0;

  virtual ~AstBuilderTestAPI() = default;
};

AstBuilderTestAPI *CreateBuilder();
std::unique_ptr<AST::Node> Parse(Lexer *lexer, ErrorHandler *herr);

}  // namespace enigma::parsing

#endif  // ENIGMA_COMPILER_PARSING_PARSER_h
