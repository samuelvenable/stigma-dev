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

#ifndef ENIGMA_COMPILER_PARSING_AST_h
#define ENIGMA_COMPILER_PARSING_AST_h

#include "win32_macro_guard.h"
#include "full_type.h"
#include "error_reporting.h"
#include "lexer.h"
#include "tokens.h"
#include "darray.h"

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct ParsedScope;  // object_storage.h
struct CompileState;

namespace enigma::parsing {

class AST {
 public:
  enum class NodeType {
    ERROR = 0,
    BLOCK = 1,
    BINARY_EXPRESSION,
    UNARY_PREFIX_EXPRESSION,
    UNARY_POSTFIX_EXPRESSION,
    TERNARY_EXPRESSION,
    LAMBDA_EXPRESSION,
    TYPE_SPECIFIER_SEQ,
    DECL_SPEC_LIST,
    SIZEOF, ALIGNOF, CAST,
    NEW, DELETE,
    PARENTHETICAL, ARRAY,
    IDENTIFIER, SCOPE_ACCESS, LITERAL, FUNCTION_CALL,
    IF, FOR, WHILE, DO, WITH, REPEAT, SWITCH, CASE, DEFAULT,
    BREAK, CONTINUE, RETURN, DECLARATION, INIT_DECLARATOR, INITIALIZER,
    DECLARATOR_CLAUSE
  };

  struct Node;
  class Visitor;
  typedef std::unique_ptr<Node> PNode;

  struct Node {
    NodeType type;

    /// Visit this node with the specified visitor. If the visit routine
    /// for this node returns true, visit children of this node as well.
    virtual void RecurusiveVisit(Visitor &visitor) = 0;
    /// Visit children of this node (via RecurusiveVisit) with the given
    /// visitor. Do not invoke the visitor on this node itself.
    virtual void RecursiveSubVisit(Visitor &visitor) = 0;
    /// Cast the node to a given type
    template <typename T>
    T* As() {
        return dynamic_cast<T*>(this);
    }
    // Helper function that calls the appropriate Visitor function for this node type
    virtual bool accept(Visitor& visitor) = 0;

    Node(NodeType t = NodeType::ERROR): type(t) {}
    virtual ~Node() = default;

   protected:
    template<typename... SubNodes>
    void RV(Visitor &visitor, const SubNodes &...nodes);

   private:
    // unique_ptr<T> is invariant for lvalue conversions even when T : Node,
    // so RV()'s forwarding to RVF needs to accept any unique_ptr<T : Node>
    // (and any vector thereof) directly rather than via the base PNode type.
    template<typename T>
    void RVF(Visitor &visitor, const std::unique_ptr<T> &single_node) {
      if (single_node) single_node->RecurusiveVisit(visitor);
    }
    template<typename T>
    void RVF(Visitor &visitor, const std::vector<std::unique_ptr<T>> &node_list) {
      for (const auto &node : node_list) node->RecurusiveVisit(visitor);
    }
  };

  template<NodeType kType> struct TypedNode : Node {
    TypedNode(): Node(kType) {}
  };

  struct ConstValue {
    /// Hardware representation of supported values.
    typedef std::variant<long double, long long, std::string> HardwareValue;
    HardwareValue value;
    TokenType type;

    /// When processed from a C++-compatible token, this is the original
    /// spelling. Useful for preserving floats like 0.123.
    /// When the original spelling is not available, this serves as a
    /// cache of the latest computed spelling.
    std::optional<std::string> literal_representation;

    // TODO: Make this parse the data correctly
    ConstValue(const Token &t): value{std::string{t.content}}, type{t.type} {}
    std::string ToCppLiteral() const { return ""; }
    std::string ToCppLiteral() { return "";}
  };

#define BASIC_NODE_ROUTINES(name)                                            \
  bool accept(Visitor &visitor) final { return visitor.Visit##name(*this); } \
  void RecurusiveVisit(Visitor &visitor) final {                             \
    if (visitor.Visit##name(*this)) RecursiveSubVisit(visitor);              \
  }                                                                          \
  void RecursiveSubVisit(Visitor &visitor) final

  // Sentinel for a sub-tree that failed to parse. Carries the token where
  // parsing went off the rails; the diagnostic itself is already on `herr`.
  // Visitors no-op by default (return true so traversal continues).
  struct SyntaxError : TypedNode<NodeType::ERROR> {
    Token origin;

    BASIC_NODE_ROUTINES(SyntaxError);

    SyntaxError() noexcept = default;
    explicit SyntaxError(Token origin): origin{std::move(origin)} {}
  };

  // Simple block of code, containing zero or more statements.
  // The root node of any piece of code will be a block node.
  struct CodeBlock : TypedNode<NodeType::BLOCK> {
    // Individual statements or 
    std::vector<PNode> statements;

    BASIC_NODE_ROUTINES(CodeBlock);

    CodeBlock() noexcept = default;
    CodeBlock(std::vector<PNode> statements): statements{std::move(statements)} {}
  };

  struct Operation{
    TokenType type;
    std::string token;

    Operation(TokenType type_, std::string token_):type(type_), token(token_){}
  };

  // Binary expressions; generally top-level will be "foo = expression"
  struct BinaryExpression : TypedNode<NodeType::BINARY_EXPRESSION> {
    PNode left, right;
    Operation operation;

    BASIC_NODE_ROUTINES(BinaryExpression);

    BinaryExpression(PNode left_, PNode right_, Operation operation_):
        left(std::move(left_)), right(std::move(right_)),
        operation(operation_) {}
  };

  // Function call expression, foo(bar)
  struct FunctionCallExpression : TypedNode<NodeType::FUNCTION_CALL> {
    PNode function;
    std::vector<PNode> arguments;

    BASIC_NODE_ROUTINES(FunctionCallExpression);

    FunctionCallExpression(PNode function_, std::vector<PNode> &&arguments_): function{std::move(function_)}, arguments{std::move(arguments_)} {}
  };

  // Unary prefix expressions; generally top-level will be "++varname"
  struct UnaryPrefixExpression : TypedNode<NodeType::UNARY_PREFIX_EXPRESSION> {
    PNode operand;
    Operation operation;

    // Returns true iff this operation can be used in a declaration.
    // (e.g. `*x`, `&x`, `&&x`)
    bool CanBeTypeSpecifier() const;

    BASIC_NODE_ROUTINES(UnaryPrefixExpression);

    UnaryPrefixExpression(PNode operand_, Operation operation_):
        operand(std::move(operand_)), operation(operation_) {}
  };

  // Unary postfix expression
  struct UnaryPostfixExpression : TypedNode<NodeType::UNARY_POSTFIX_EXPRESSION> {
    PNode operand;
    Operation operation;

    BASIC_NODE_ROUTINES(UnaryPostfixExpression);

    UnaryPostfixExpression(PNode operand_, Operation operation_):
        operand(std::move(operand_)), operation(operation_) {}
  };

  // Ternary expression; the only one is ?:
  struct TernaryExpression : TypedNode<NodeType::TERNARY_EXPRESSION> {
    PNode condition;
    PNode true_expression;
    PNode false_expression;

    BASIC_NODE_ROUTINES(TernaryExpression);

    TernaryExpression(PNode condition_, PNode true_expression_, PNode false_expression_):
      condition{std::move(condition_)}, true_expression{std::move(true_expression_)}, false_expression{std::move(false_expression_)} {}
  };

  // A run of decl-specifier keywords (`unsigned long const ...`). Order is
  // preserved for pretty-printer fidelity; `flags` is the JDI-bitmask sum
  // for semantic-phase consumers.
  struct DeclSpecList : TypedNode<NodeType::DECL_SPEC_LIST> {
    std::vector<Token> specs;
    std::size_t flags = 0;

    BASIC_NODE_ROUTINES(DeclSpecList);

    DeclSpecList() = default;
    DeclSpecList(std::vector<Token> specs_, std::size_t flags_):
        specs(std::move(specs_)), flags(flags_) {}
  };

  // Models the C++ `type-specifier-seq` production: a base type name plus its
  // run of cv/sign/length specifiers (`unsigned long const …`). It is NOT a
  // full `type-id` — a type-id is `type-specifier-seq` + an (abstract-)declarator
  // (the `*` `&` `[]` `()` part). That declarator is modeled separately: today
  // on `InitDeclarator` (declarator_expr) or smuggled through `type_info`, and
  // in the unified design by a `DeclaratorClause` node pairing this seq with a
  // declarator tree. So a cast/sizeof/new target is a `DeclaratorClause` whose
  // specifier part is one of these; this node alone is just the spec run.
  //
  // The base type is held as a resolved `def` (JDI definition) OR an unresolved
  // `id_expression` awaiting semantic resolution; the semantic phase decides
  // which applies, and both may be populated during a transitional pass. The
  // "seq" is `declspecs` — a `DeclSpecList`, i.e. an ordered `std::vector<Token>`.
  //
  // Note: `def` is named for what it carries (a JDI definition pointer), not
  // "resolved final type" — a `def` pointing at a typedef still leaves that
  // typedef's own declarator chain to be walked.
  //
  // `type_info` is the cached/synthesized FullType (the JDI-bridge layer's
  // form of this same type). It's populated either at construction (when a
  // caller already has a FullType in hand) or, post-4e, lazily by
  // `to_jdi_fulltype()` walking declspecs + declarator-expr-tree. Not
  // transitional — FullType stays as ENIGMA's intra-system bridge to JDI.
  struct TypeSpecifierSeq : TypedNode<NodeType::TYPE_SPECIFIER_SEQ> {
    jdi::definition *def = nullptr;
    PNode id_expression;
    std::unique_ptr<DeclSpecList> declspecs;
    FullType type_info;
    enum class Scope { DEFAULT, GLOBAL, LOCAL } scope = Scope::DEFAULT;

    BASIC_NODE_ROUTINES(TypeSpecifierSeq);

    // Primary constructor: phase-2 callers use this. id_expression is
    // optional (nullable); declspecs is optional (nullable). `type_info` is
    // left empty; populate via lazy synthesis or a follow-up assignment.
    TypeSpecifierSeq(jdi::definition *def_, PNode id_exp, std::unique_ptr<DeclSpecList> specs, Scope scope_ = Scope::DEFAULT):
        def(def_), id_expression(std::move(id_exp)), declspecs(std::move(specs)), scope(scope_) {}

    // "Construct from already-built FullType" overloads — used by call sites
    // that produce a FullType in the legacy spec-seq + declarator parse
    // (still the path for things like new-expression bare type-ids until 4e
    // gives us native declarator-expression-tree parsing). The FullType
    // populates the type_info cache.
    TypeSpecifierSeq(PNode id_exp, FullType type_, Scope scope_):
        def(type_.def), id_expression(std::move(id_exp)), type_info(std::move(type_)), scope(scope_) {}
    TypeSpecifierSeq(PNode id_exp, FullType type_):
        TypeSpecifierSeq(std::move(id_exp), std::move(type_), Scope::DEFAULT) {}
    TypeSpecifierSeq(PNode id_exp): id_expression(std::move(id_exp)) {}

    // JDI bridge. Used by sites that need to feed this spec-seq into the legacy
    // JDI machinery (template-arg keys, function-parameter ref-stacks). Today it
    // delegates to the transitional FullType in `type_info`; once the unified
    // declarator build-out gives us a declarator tree on the owning clause
    // (DeclaratorClause / InitDeclarator), this synthesizes the jdi::full_type
    // from declspecs + that tree directly. Non-const because the underlying
    // Declarator::to_jdi_refstack walk is non-const.
    jdi::full_type to_jdi_fulltype();
  };

  // Lambda expression: x => x + 10;
  struct LambdaExpression : TypedNode<NodeType::LAMBDA_EXPRESSION> {
    PNode parameters;
    PNode body;

    BASIC_NODE_ROUTINES(LambdaExpression);

    LambdaExpression(PNode parameters_, PNode body_):
      parameters{std::move(parameters_)}, body{std::move(body_)} {}
  };

  // Sizeof expression
  struct SizeofExpression : TypedNode<NodeType::SIZEOF> {
    enum class Kind { EXPR, VARIADIC, TYPE } kind;
    PNode argument;

    BASIC_NODE_ROUTINES(SizeofExpression);

    explicit SizeofExpression(PNode arg): kind{Kind::EXPR}, argument{std::move(arg)} {}
  };

  // Alignof expression
  struct AlignofExpression : TypedNode<NodeType::ALIGNOF> {
    PNode type;

    BASIC_NODE_ROUTINES(AlignofExpression);

    explicit AlignofExpression(PNode type_): type{std::move(type_)} {}
  };

  // Cast expressions
  struct CastExpression : TypedNode<NodeType::CAST> {
    // Note that FUNCTIONAL casts are now modeled as initializing a TypeSpecifierSeq.
    enum class Kind { C_STYLE, STATIC, DYNAMIC, REINTERPRET, CONST } kind;
    PNode type;
    PNode expr;

    static const std::vector<std::string> KindNames;

    BASIC_NODE_ROUTINES(CastExpression);
    static std::string KindToString(Kind k);

    CastExpression(const Token &token, PNode type_, PNode expr_):
       type{std::move(type_)}, expr{std::move(expr_)} {
      switch (token.type) {
        case TT_ENDPARENTH:     kind = Kind::C_STYLE; break;
        case TT_STATIC_CAST:      kind = Kind::STATIC; break;
        case TT_DYNAMIC_CAST:     kind = Kind::DYNAMIC; break;
        case TT_REINTERPRET_CAST: kind = Kind::REINTERPRET; break;
        case TT_CONST_CAST:       kind = Kind::CONST; break;
        default:                  break;
      }
    }

    CastExpression(Kind kind_, const Token &token, PNode type_, PNode expr_):
      CastExpression(token, std::move(type_), std::move(expr_)) {
      kind = kind_;
    }
  };

  // No-op tree node that allows true-to-original pretty printing and
  // establishes a formal place for empty (null) nodes in a complete tree.
  struct Parenthetical : TypedNode<NodeType::PARENTHETICAL> {
    PNode expression;

    BASIC_NODE_ROUTINES(Parenthetical);

    Parenthetical(PNode expression_): expression(std::move(expression_)) {}
  };

  struct Array : TypedNode<NodeType::ARRAY> {
    std::vector<PNode> elements;

    BASIC_NODE_ROUTINES(Array);

    Array(std::vector<PNode> &&elements_): elements(std::move(elements_)) {}
  };

  struct IdentifierAccess : TypedNode<NodeType::IDENTIFIER> {
    // We can access identifiers declared either in C++ or EDL
    enum class Kind { EDL, CPP } kind;
    std::variant<FullType*, jdi::definition*> type;
    // When this IdentifierAccess is the leaf of a declarator chain (on
    // DeclarationStatement::Declaration::declarator), `name.content` may be
    // empty — that encodes an *abstract* declarator (no name, e.g. the type
    // in `(int*)x` or an unnamed function parameter). Consumers that read
    // the name should tolerate empty content; the existing convention in
    // parser.cpp:678 etc. already does.
    Token name;

    BASIC_NODE_ROUTINES(IdentifierAccess);

    IdentifierAccess(FullType *type, Token name): kind{Kind::EDL}, type{type}, name{name} {}
    IdentifierAccess(jdi::definition *type, Token name): kind{Kind::CPP}, type{type}, name{name} {}
    IdentifierAccess(Token name): kind{Kind::CPP}, type{}, name{name} {}
  };

  struct Literal : TypedNode<NodeType::LITERAL> {
    ConstValue value;

    BASIC_NODE_ROUTINES(Literal);

    Literal(const Token &token): value{token} {}
  };
  
  struct IfStatement : TypedNode<NodeType::IF> {
    PNode condition;
    PNode true_branch, false_branch;
    bool not_condition;

    BASIC_NODE_ROUTINES(IfStatement);

    IfStatement(PNode condition_, PNode true_branch_, PNode false_branch_, bool not_condition_): condition{std::move(condition_)},
        true_branch{std::move(true_branch_)}, false_branch{std::move(false_branch_)}, not_condition(not_condition_){}
  };

  struct ForLoop : TypedNode<NodeType::FOR> {
    PNode assignment, condition, increment;
    PNode body;

    BASIC_NODE_ROUTINES(ForLoop);


    ForLoop(PNode assignment_, PNode condition_, PNode increment_, PNode body_): assignment{std::move(assignment_)},
        condition{std::move(condition_)}, increment{std::move(increment_)}, body{std::move(body_)} {}
  };

  struct WhileLoop : TypedNode<NodeType::WHILE> {
    PNode condition;
    PNode body;

    enum class Kind { WHILE, UNTIL, REPEAT } kind;

    BASIC_NODE_ROUTINES(WhileLoop);

    WhileLoop(PNode condition_, PNode body_, Kind kind_): condition{std::move(condition_)},
        body{std::move(body_)}, kind{kind_} {}
  };

  struct DoLoop : TypedNode<NodeType::DO> {
    PNode body;
    PNode condition;
    bool is_until;

    BASIC_NODE_ROUTINES(DoLoop);

    DoLoop(PNode body_, PNode condition_, bool until): body{std::move(body_)}, condition{std::move(condition_)},
        is_until(until) {}
  };

  struct CaseStatement : TypedNode<NodeType::CASE> {
    PNode value;
    std::unique_ptr<AST::CodeBlock> statements;

    BASIC_NODE_ROUTINES(CaseStatement);

    CaseStatement(PNode value, std::unique_ptr<AST::CodeBlock> statements): value{std::move(value)},
                                                                             statements{std::move(statements)} {}
  };

  struct DefaultStatement : TypedNode<NodeType::DEFAULT> {
    std::unique_ptr<AST::CodeBlock> statements;

    BASIC_NODE_ROUTINES(DefaultStatement);

    DefaultStatement(std::unique_ptr<AST::CodeBlock> statements): statements{std::move(statements)} {}
  };

  struct SwitchStatement : TypedNode<NodeType::SWITCH> {
    PNode expression;
    std::unique_ptr<AST::CodeBlock> body;
    // Need to track these because case labels must be unique
    // and there can only be one default label.
    //
    // Use @c std::size_t as vector addresses are not stable thus pointers can cause bugs
    std::unordered_map<ConstValue::HardwareValue, std::size_t> cases;
    std::optional<std::size_t> default_branch = std::nullopt;

    BASIC_NODE_ROUTINES(SwitchStatement);
  };

  struct ReturnStatement : TypedNode<NodeType::RETURN> {
    // Optional: the return value. Default: T()
    PNode expression;
    bool is_exit;

    BASIC_NODE_ROUTINES(ReturnStatement);

    ReturnStatement(PNode expression_, bool is_exit_): expression{std::move(expression_)}, is_exit{is_exit_} {}
  };

  struct BreakStatement : TypedNode<NodeType::BREAK> {
    // Optional: the number of nested loops to break out of (default = 1)
    PNode count;

    BASIC_NODE_ROUTINES(BreakStatement);

    explicit BreakStatement(PNode count_): count{std::move(count_)} {}
  };

  struct ContinueStatement : TypedNode<NodeType::CONTINUE> {
    // Optional: the number of nested loops to continue past (default = 1)
    PNode count;

    BASIC_NODE_ROUTINES(ContinueStatement);

    explicit ContinueStatement(PNode count_): count{std::move(count_)} {}
  };

  struct WithStatement : TypedNode<NodeType::WITH> {
    PNode object;
    PNode body;

    BASIC_NODE_ROUTINES(WithStatement);

    WithStatement(PNode object_, PNode body_): object{std::move(object_)}, body{std::move(body_)} {}
  };

  enum class DeclaratorType {
    ABSTRACT, NON_ABSTRACT, MAYBE_ABSTRACT
  };

  struct Initializer;
  struct DeclaratorClause;

  using InitializerNode = std::unique_ptr<Initializer>;

  struct Initializer : TypedNode<NodeType::INITIALIZER> {
    enum class Kind {
      ASSIGN,          // = expr
      EXPR,            // expr (also used for pack expansions like args...)
      BRACE,           // { ... }
      PAREN            // ( ... )
    } kind;

    PNode target;
    std::vector<PNode> values;
    BASIC_NODE_ROUTINES(Initializer);

    Initializer(Kind k, PNode target, std::vector<PNode> vals):
      kind(k), target(std::move(target)), values(std::move(vals)) {}
  };

  // New expression
  struct NewExpression : TypedNode<NodeType::NEW> {
    bool is_global;
    bool is_array;
    std::vector<PNode> placement_args;
    std::unique_ptr<DeclaratorClause> type;
    std::unique_ptr<Initializer> initializer;

    BASIC_NODE_ROUTINES(NewExpression);

    NewExpression(bool is_global, bool is_array, std::vector<PNode> placement_args, std::unique_ptr<DeclaratorClause> type,
                  std::unique_ptr<Initializer> initializer):
      is_global{is_global}, is_array{is_array}, placement_args{std::move(placement_args)}, type{std::move(type)},
      initializer{std::move(initializer)} {}
  };

  // Delete expression
  struct DeleteExpression : TypedNode<NodeType::DELETE> {
    bool is_global;
    bool is_array;
    PNode expression;

    BASIC_NODE_ROUTINES(DeleteExpression);

    DeleteExpression(bool is_global, bool is_array, PNode expression): is_global{is_global}, is_array{is_array},
                                                                       expression{std::move(expression)} {}
  };

  // One init-declarator (the `<declarator> [= <init>]` half of a declaration).
  // `name` is the declared identifier (source of truth — written directly by
  // the parser). `declarator_expr` is the AST-layer declarator-as-expression-
  // tree describing the type-modifier chain (pointers, refs, array bounds,
  // function-params). `declarator` is the legacy FullType (JDI-bridge) form;
  // post-4D it's still populated for backwards-compatibility while consumers
  // migrate, and is removed in 4-E along with the rest of the Declarator path.
  struct InitDeclarator : TypedNode<NodeType::INIT_DECLARATOR> {
    Token name;
    std::unique_ptr<FullType> declarator;
    PNode declarator_expr;
    InitializerNode init;

    BASIC_NODE_ROUTINES(InitDeclarator);

    InitDeclarator() noexcept = default;
    InitDeclarator(Token name, FullType declarator, InitializerNode init):
      name{std::move(name)},
      declarator{std::make_unique<FullType>(std::move(declarator))}, init{std::move(init)} {}
    InitDeclarator(Token name, FullType declarator, PNode declarator_expr_, InitializerNode init):
      name{std::move(name)},
      declarator{std::make_unique<FullType>(std::move(declarator))},
      declarator_expr{std::move(declarator_expr_)}, init{std::move(init)} {}
  };

  // A type-specifier-seq paired with its declarator(s): the unified shape that
  // can sit in expression position (cast/sizeof/alignof/new targets, and later
  // function-params/arrow-fns/tuples) as well as back a DeclarationStatement.
  // `declarators` holds a single (possibly abstract) InitDeclarator for a lone
  // type-id, or several for an init-declarator-list. An abstract declarator is
  // an InitDeclarator whose declarator_expr bottoms out in an empty-name leaf
  // (see make_abstract_operand in the parser). Context-illegal combinations
  // (e.g. a name or initializer inside a sizeof type-id) are left for the
  // semantic phase to reject, not screened out here -- the parser stays
  // context-free, per the types-as-trees rule.
  struct DeclaratorClause : TypedNode<NodeType::DECLARATOR_CLAUSE> {
    std::unique_ptr<TypeSpecifierSeq> specifiers;
    std::vector<std::unique_ptr<InitDeclarator>> declarators;

    BASIC_NODE_ROUTINES(DeclaratorClause);

    DeclaratorClause(std::unique_ptr<TypeSpecifierSeq> specifiers_,
                     std::vector<std::unique_ptr<InitDeclarator>> declarators_):
        specifiers(std::move(specifiers_)), declarators(std::move(declarators_)) {}
  };

  struct DeclarationStatement: TypedNode<NodeType::DECLARATION> {
    enum class StorageClass {
      TEMPORARY,
      LOCAL,
      GLOBAL,
    };

    // A declaration is its DeclaratorClause (shared type-specifier-seq + the
    // init-declarator-list) plus how the surrounding statement scopes it.
    std::unique_ptr<DeclaratorClause> clause;
    StorageClass storage_class;
    static const std::vector<std::string> StorageNames; // what is the use of this?

    BASIC_NODE_ROUTINES(DeclarationStatement);
    static std::string StorageToString(StorageClass st);

    DeclarationStatement(StorageClass sc, std::unique_ptr<DeclaratorClause> clause_):
        clause{std::move(clause_)}, storage_class{sc} {}
  };

  class Visitor {
   public:
    virtual bool DefaultVisit(Node &node) {
      (void)node;
      return true;
    }
    virtual bool VisitSyntaxError(SyntaxError &node){ return DefaultVisit(node); }
    virtual bool VisitCodeBlock(CodeBlock &node){ return DefaultVisit(node); }
    virtual bool VisitBinaryExpression(BinaryExpression &node){ return DefaultVisit(node); }
    virtual bool VisitFunctionCallExpression(FunctionCallExpression &node){ return DefaultVisit(node); }
    virtual bool VisitUnaryPrefixExpression(UnaryPrefixExpression &node){ return DefaultVisit(node); }
    virtual bool VisitUnaryPostfixExpression(UnaryPostfixExpression &node){ return DefaultVisit(node); }
    virtual bool VisitTernaryExpression(TernaryExpression &node){ return DefaultVisit(node); }
    virtual bool VisitTypeSpecifierSeq(TypeSpecifierSeq &node){ return DefaultVisit(node); }
    virtual bool VisitDeclSpecList(DeclSpecList &node){ return DefaultVisit(node); }
    virtual bool VisitLambdaExpression(LambdaExpression &node){ return DefaultVisit(node); }
    virtual bool VisitSizeofExpression(SizeofExpression &node){ return DefaultVisit(node); }
    virtual bool VisitAlignofExpression(AlignofExpression &node){ return DefaultVisit(node); }
    virtual bool VisitCastExpression(CastExpression &node){ return DefaultVisit(node); }
    virtual bool VisitParenthetical(Parenthetical &node){ return DefaultVisit(node); }
    virtual bool VisitArray(Array &node){ return DefaultVisit(node); }
    virtual bool VisitIdentifierAccess(IdentifierAccess &node){ return DefaultVisit(node); }
    virtual bool VisitLiteral(Literal &node){ return DefaultVisit(node); }
    virtual bool VisitIfStatement(IfStatement &node){ return DefaultVisit(node); }
    virtual bool VisitForLoop(ForLoop &node){ return DefaultVisit(node); }
    virtual bool VisitWhileLoop(WhileLoop &node){ return DefaultVisit(node); }
    virtual bool VisitDoLoop(DoLoop &node){ return DefaultVisit(node); }
    virtual bool VisitCaseStatement(CaseStatement &node){ return DefaultVisit(node); }
    virtual bool VisitDefaultStatement(DefaultStatement &node){ return DefaultVisit(node); }
    virtual bool VisitSwitchStatement(SwitchStatement &node){ return DefaultVisit(node); }
    virtual bool VisitReturnStatement(ReturnStatement &node){ return DefaultVisit(node); }
    virtual bool VisitBreakStatement(BreakStatement &node){ return DefaultVisit(node); }
    virtual bool VisitContinueStatement(ContinueStatement &node){ return DefaultVisit(node); }
    virtual bool VisitWithStatement(WithStatement &node){ return DefaultVisit(node); }
    virtual bool VisitInitializer(Initializer &node){ return DefaultVisit(node); }
    virtual bool VisitNewExpression(NewExpression &node){ return DefaultVisit(node); }
    virtual bool VisitDeleteExpression(DeleteExpression &node){ return DefaultVisit(node); }
    virtual bool VisitDeclarationStatement(DeclarationStatement &node){ return DefaultVisit(node); }
    virtual bool VisitInitDeclarator(InitDeclarator &node){ return DefaultVisit(node); }
    virtual bool VisitDeclaratorClause(DeclaratorClause &node){ return DefaultVisit(node); }
    virtual bool Visit(PNode &node) {
      return node->accept(*this);
    }
  };

  class CppPrettyPrinter : public AST::Visitor {
    std::ofstream *of;
    bool print_type;
    bool is_script;
    const LanguageFrontend *language_fe = nullptr;

   public:
    CppPrettyPrinter();
    CppPrettyPrinter(const LanguageFrontend *lfe);
    CppPrettyPrinter(std::ofstream &ofs, const LanguageFrontend *lfe, bool is_script);
    void print(std::string code);
    void PrintSemiColon(PNode &node);
    std::string GetPrintedCode();
    bool VisitCode(CodeBlock &node);
    bool VisitCodeBlock(CodeBlock &node);
    bool VisitDot(BinaryExpression &node);
    bool VisitBinaryExpression(BinaryExpression &node);
    bool VisitFunctionCallExpression(FunctionCallExpression &node);
    bool VisitUnaryPrefixExpression(UnaryPrefixExpression &node);
    bool VisitUnaryPostfixExpression(UnaryPostfixExpression &node);
    bool VisitTernaryExpression(TernaryExpression &node);
    bool VisitTypeSpecifierSeq(TypeSpecifierSeq &node);
    bool VisitDeclSpecList(DeclSpecList &node);
    bool VisitLambdaExpression(LambdaExpression &node);
    bool VisitFullType(FullType &node, bool print_type = true);
    bool VisitSizeofExpression(SizeofExpression &node);
    bool VisitAlignofExpression(AlignofExpression &node);
    bool VisitCastExpression(CastExpression &node);
    bool VisitParenthetical(Parenthetical &node);
    bool VisitArray(Array &node);
    bool VisitIdentifierAccess(IdentifierAccess &node);
    bool VisitLiteral(Literal &node);
    bool VisitIfStatement(IfStatement &node);
    bool VisitForLoop(ForLoop &node);
    bool VisitWhileLoop(WhileLoop &node);
    bool VisitDoLoop(DoLoop &node);
    bool VisitCaseStatement(CaseStatement &node);
    bool VisitDefaultStatement(DefaultStatement &node);
    bool VisitSwitchStatement(SwitchStatement &node);
    bool VisitReturnStatement(ReturnStatement &node);
    bool VisitBreakStatement(BreakStatement &node);
    bool VisitContinueStatement(ContinueStatement &node);
    bool VisitWithStatement(WithStatement &node);
    bool VisitInitializer(Initializer &node);
    bool VisitNewExpression(NewExpression &node);
    bool VisitDeleteExpression(DeleteExpression &node);
    bool VisitDeclarationStatement(DeclarationStatement &node);
    bool VisitInitDeclarator(InitDeclarator &node);
    bool VisitDeclaratorClause(DeclaratorClause &node);
  };

  // Used to adapt to current single-error syntax checking interface.
  ErrorCollector herr;
  // A lexed (tokenized) view of the code.
  const std::shared_ptr<Lexer> lexer;
  // The raw input code, owned by the lexer.
  const std::string &code;
  // Lambda function to get nodes types as strings 
  static const std::vector<std::string> NodesNames;

  bool HasError() { return !herr.errors.empty(); }
  std::string ErrorString() {
    if (herr.errors.empty()) return "No error";
    return herr.errors.front().ToString();
  }

  // Returns true if there's no actual executable code in this AST.
  bool empty() const;

  // Utility routine: Apply this AST to a specified instance.
  void ApplyTo(int instance_id);

  // Extract declarations from this AST into the specified scope.
  void ExtractDeclarations(ParsedScope *destination_scope, CompileState *cs);

  // Pretty-prints this code to a stream with the given base indentation.
  // void PrettyPrint(std::ofstream &of, int base_indent = 2) const;

  // Writes this code in C++ format to the given stream with the given
  // base indentation. The compiler will attempt to preserve token line
  // numbers and will emit a #file directive for the original source.
  //
  // The caller is responsible for having already printed applicable
  // function declarations and opening braces, statements, etc, and for
  // printing the closing statements and braces afterward.
  void WriteCppToStream(std::ofstream &of, int base_indent = 2, bool is_script = false) const;

  // Parses the given code, returning an AST*. The resulting AST* is never null.
  // If syntax errors were encountered, they are stored within the AST.
  static AST Parse(std::string_view code, const ParseContext* ctex);

  void VisitNodes(Visitor &visitor) {
    if (root_) root_->accept(visitor);
  }

  // Get the node type as a string
  static std::string NodeToString(NodeType nt);

  // Disallow copy. Our tokens point into our code.
  AST(const AST &) = delete;
  AST(AST &&other) = default;

 private:
  std::unique_ptr<Node> root_;
  // When specified, emits code to apply to a specific instance.
  std::optional<int> apply_to_;
  

  // Constructs an AST from the code it will parse. Does not initiate parse.
  AST(std::string_view code_, const ParseContext *ctex):
      lexer(std::make_unique<Lexer>(code_, ctex, &herr)),
      code(lexer->GetCode()) {}
};

// AST→JDI bridge: walk a declarator-expression-tree into a jdi::ref_stack.
// Returns false on a malformed sub-tree (unsupported node type, missing array
// size, etc.). Combine with a TypeSpecifierSeq's def/flags to produce a full jdi::full_type.
bool walk_declarator_expr(AST::Node *expr, jdi::ref_stack &result);

}  // namespace enigma::parsing

#endif  // ENIGMA_COMPILER_PARSING_AST_h
