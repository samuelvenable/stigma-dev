# JDI2: a denotational C++ index, populated by libclang

Ruled 2026-07-05 (Josh + Claude). Supersedes both "adopt the thin clang
adapter as-is" and "rewrite JDI's parser."

## The one-paragraph version

JDI's parser dies; JDI's data model is the product. libclang parses the
engine (and definition pages) and *populates* JDI's Storage structures --
`definition`, `definition_scope`, `definition_class`, `definition_typed`,
`definition_template` with its `arg_key`-keyed specialization map -- each
now carrying its originating `CXCursor` (the `CXTranslationUnit` outlives
the context, so cursors stay valid). Consumers keep speaking `jdi::`;
nothing above the population layer changes. ENIGMA holds its own EDL AST
forever; JDI holds the definition tree over C++ forever; language adapters
export the AST wherever (g++ today; clang/emscripten/LLVM if the backend
dream calls). JDI remains a separable product: a denotational engine that
answers YCM-class questions (`foo[bar].baz`) from its index, without
reparsing the world.

## Why not the alternatives

- **Sema as a collaborator**: Sema is welded to Clang's own Parser (parser
  action callbacks); there is no stable "resolve my AST in your TU" entry
  point. The existence proof, LLDB's expression evaluator, is the
  cautionary tale: unstable C++ API, version-locked, a team to maintain
  it. libclang exposes no Sema at all. We refuse the C++ API on purpose.
- **The thin adapter as final home**: Greg/Opus's `ClangDefinition*` layer
  is a correctly-shaped *populator* -- name-map linkage, flags, scopes,
  overloads -- but it drops exactly the meat ENIGMA's workload needs
  (templates, specializations, declarator composition, evaluation), and
  `ClangDefinitionTyped` is just `definition_typed` with a cursor in its
  pocket. Keep its traversal code; retarget its destination.
- **Clang's query model**: refuses to let an id-expression denote one
  thing. ENIGMA's chained-dot lowering (and every human reading code)
  requires exactly that. The denotational question set is the permanent
  workload, so the index is the architecture, not a cache bolted on.

Side benefit of keeping Storage: `jdi::full_type`/`ref_stack` survive, so
ENIGMA's AST bridge (`to_jdi_fulltype`, `walk_declarator_expr`) compiles
untouched.

## Expression doctrine: guarantee, escalate, never approximate

JDI historically *skipped* expressions (read past them). C++11+ ended
that: choosing a specialization can require evaluating arbitrary
constexpr code. The rule set:

1. **Model what we model exactly.** Within the index (names, scopes,
   members, declared specializations via refreshed `arg_key`), answers
   are exact by construction. Note `vector<bool>` is a *declared*
   specialization in libstdc++ -- a specialization-aware index answers
   the classic pathological case with no evaluation at all.
2. **Pocket what we don't -- and reason through it directly.** Expressions
   JDI once discarded become thin nodes holding the Clang cursor. The thin
   node carries a partial evaluator over the Clang AST itself:
   `eval(cursor, env) -> value | ESCALATE`, walking cursor children for
   the cases it knows (integer literals, the shift/arithmetic operator
   set via `clang_getCursorBinaryOperatorKind` -- pinning libclang >= 17,
   sizeof of indexed types, DeclRefs resolving to indexed constants or to
   template parameters bound in `env` by the arg_key at lookup time).
   No JDI-side expression IR: the cursor is the representation. Every
   unhandled node kind returns the sentinel. This evaluator is the
   framework the long-term ambition rests on: the 97% case fast and
   exact, the long tail degrading gracefully to full evaluation -- the
   design we would want the Clang debugger tooling world to adopt, where
   added effort widens the handled-node-kind table rather than
   re-architecting.
3. **Escalate on demand, by sentinel.** Evaluating a thin node returns an
   ESCALATE sentinel that propagates up the evaluation stack; the top
   level then resolves via, in order: `clang_Cursor_Evaluate` (libclang's
   constant evaluator), existing instantiations in the TU, and finally a
   **probe TU** -- print a snippet forcing the instantiation (we own a
   pretty printer; source text is the stable interface) and reparse.
   Make probes fast by treating the engine TU as a precompiled preamble.
   A compile-time raytracer escalates once per decl, where it belongs.
4. **Grow the evaluator's table under the sentinel.** When telemetry
   shows hundreds of escalations on some node kind, add its case to the
   cursor evaluator. Escalation frequency is the roadmap; the case table
   is where outside contribution lands.
5. **Cache escalation results JDI-side.** C++ constant evaluation is
   deterministic and side-effect-free by construction (UB and I/O are
   ill-formed in constant expressions; even C++20 constexpr `new` must be
   transient, so purity holds). The cache key must carry the context that
   legitimately changes answers: (expression, instantiation/template
   arguments, TU identity = engine config + defines + header set).
   `is_constant_evaluated()`/`if consteval` split by evaluation context,
   not by time -- key on "constant context" and the cache is sound.
   Invalidate with the TU (config or definition-page change).

MVP requires none of steps 3-5: skeleton population + declared-
specialization lookup covers everything the current corpus and sema
tiers consume. `new` in constexpr stays escalate-only forever; that
depth is why debugger teams have headcount.

## Tranche plan (budget-aware)

Verification gate for every tranche: ENIGMA's unit suite (365+) plus the
4-SOG runtime gauntlet, snapshot diffed; subagent work independently
verified before acceptance.

- **T0 (mechanical, delegate):** land clang_adapter files in a staging
  namespace on NewENIGMA2026; link libclang; compile beside JDI, wired to
  nothing. Zero behavior change.
- **T1 (mechanical, delegate; seam designed first):** add `CXCursor` +
  TU-lifetime plumbing to `jdi::definition`; retarget ClangContext's
  traversal to construct Storage structs. Standalone populate-and-dump
  test compares against JDI1's parse of the same headers.

  **T1 seam (pinned 2026-07-05):**
  - `jdi::definition` gains `CXCursor cursor` (null-cursor default);
    `Storage/definition.h` includes `<clang-c/Index.h>` directly -- a
    stable system C header, and JDI2 pins libclang regardless. JDI1's
    own constructors never set it; population does.
  - Lifetime rule: cursors are valid only while the populating
    `ClangContext` (owner of `CXIndex` + `CXTranslationUnit`) lives.
    Anything caching `definition*` must not outlive the context -- the
    same discipline `definitionsModified`'s context swap already
    imposes on `namespace_enigma`/`enigma_type__*`.
  - Ownership: scopes own members via the existing
    `defmap = map<string, unique_ptr<definition>>`; the populator
    constructs with `make_unique` into the parent's map. No parallel
    arena, no shared_ptr (the thin adapter's shared_ptr model retires
    with its structs).
  - Functions populate through `definition_function`'s existing
    `overload(...)` API; parameters ride an `RT_FUNCTION` ref_stack
    exactly as `walk_declarator_expr` (parsing/ast.cpp) builds them --
    that function is the canonical in-tree exemplar.
  - T1 skeleton scope: namespaces, classes, enums, typedefs, variables,
    functions with overloads + parameters. Templates get the
    DEF_TEMPLATE flag only (no definition_template construction --
    that's T4 with the arg_key refresh). The T0-disabled
    `jdi::builtin_type__int` assignments STAY disabled until T2: the
    adapter must not mutate JDI1 globals while JDI1 is still primary.
- **T2 (judgment, in-house):** cut lang_CPP over -- `main_context`
  becomes the clang-populated context; port their `jdi_utility.cpp`
  frontend impls (overload-aware parameter bounds); definitionsModified
  rewrite (include-dir assembly; definition pages via parse -- the
  enigma_user-wrap workaround is deleted, not ported, since clang does
  not scar scopes).
- **T3 (mixed):** suite + gauntlet green on clang population; unwind the
  TODO(jdi2) ledger: arg-count warnings back to errors, varaccess
  builtin-local fix against real engine types, template-param
  introspection (cursor visitor) feeding `TemplateId` classification.
- **T4+ (customer-driven):** `arg_key` refresh for modern C++ +
  specialization registration; thin expression nodes + sentinel; probe-TU
  + preamble caching; JDI-side operator evaluation per telemetry; JDI's
  own lookup-API surface for external (editor) customers -- callers
  parse, JDI answers.

Delegation tiers: mechanical tranches go to Sonnet-class subagents with
tight prompts and explicit verification steps; design seams, cutovers,
and all acceptance testing stay in-house. Escalation-frequency telemetry
and the populate-and-dump differ are the cheap instruments that keep
subagent work honest.
