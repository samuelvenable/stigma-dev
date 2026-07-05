// JDI2 T1: exercises clang_adapter::ClangContext directly, asserting that it
// populates a real jdi::Storage tree (definition/definition_scope/
// definition_class/definition_typed/definition_function) rather than the
// retired ClangDefinition* mirror. See CompilerSource/JDI/JDI2-DESIGN.md.
//
// The adapter itself is wired to nothing in the compile pipeline (lang_CPP
// is untouched); this suite is the only thing that drives it.

#include <languages/clang_adapter.h>
#include <Storage/definition.h>
#include <clang-c/Index.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Small fixture covering the T1 population surface: a namespace with a
// nested class (two members + a method), a typedef, a two-value enum, a
// global variable, an overloaded free function (one overload with a
// defaulted parameter), and a class template.
constexpr char kFixtureSource[] = R"cpp(
namespace demo_ns {
  class Widget {
   public:
    int x;
    int y;
    int sum() const;
  };
}

typedef int demo_int_t;

enum Color { RED, GREEN };

int global_counter;

void greet(int x);
void greet(int x, int y = 5);

int printf_like(const char* fmt, ...);


template <typename T>
class Box {
 public:
  T value;
};
)cpp";

std::string WriteFixture() {
  std::filesystem::path path = std::filesystem::path("/tmp") / "clang_populator_test_fixture.hpp";
  std::ofstream out(path, std::ios::trunc);
  out << kFixtureSource;
  out.close();
  return path.string();
}

// This file lives at <repo-root>/CommandLine/emake-tests/Parsing/
// clang-populator-test.cpp; used only by the disabled full-engine probe.
std::filesystem::path EnigmaRoot() {
  return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
}

}  // namespace

class ClangPopulatorTest : public ::testing::Test {
 protected:
  clang_adapter::ClangContext ctx;
  static std::string fixture_path;

  static void SetUpTestSuite() { fixture_path = WriteFixture(); }

  void SetUp() override {
    ASSERT_FALSE(fixture_path.empty());
    ctx.parse_file(fixture_path);
  }
};
std::string ClangPopulatorTest::fixture_path;

TEST_F(ClangPopulatorTest, NamespaceAndNestedClass) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);

  // The class must not be directly visible at global scope...
  EXPECT_EQ(global->find_local("Widget"), nullptr);

  // ...it belongs to the namespace.
  jdi::definition* ns_def = global->find_local("demo_ns");
  ASSERT_NE(ns_def, nullptr);
  EXPECT_TRUE(ns_def->flags & jdi::DEF_NAMESPACE);
  auto* ns_scope = dynamic_cast<jdi::definition_scope*>(ns_def);
  ASSERT_NE(ns_scope, nullptr);

  jdi::definition* widget_def = ns_scope->find_local("Widget");
  ASSERT_NE(widget_def, nullptr);
  EXPECT_EQ(widget_def->flags & (jdi::DEF_CLASS | jdi::DEF_TYPENAME),
            (unsigned)(jdi::DEF_CLASS | jdi::DEF_TYPENAME));

  auto* widget_scope = dynamic_cast<jdi::definition_scope*>(widget_def);
  ASSERT_NE(widget_scope, nullptr);

  jdi::definition* x_def = widget_scope->find_local("x");
  ASSERT_NE(x_def, nullptr);
  EXPECT_TRUE(x_def->flags & jdi::DEF_TYPED);

  jdi::definition* y_def = widget_scope->find_local("y");
  EXPECT_NE(y_def, nullptr);

  jdi::definition* sum_def = widget_scope->find_local("sum");
  ASSERT_NE(sum_def, nullptr);
  EXPECT_TRUE(sum_def->flags & jdi::DEF_FUNCTION);
}

TEST_F(ClangPopulatorTest, CursorPopulation) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);
  jdi::definition* ns_def = global->find_local("demo_ns");
  ASSERT_NE(ns_def, nullptr);

  // A definition populated from a real cursor carries a non-null one...
  EXPECT_EQ(clang_Cursor_isNull(ns_def->cursor), 0);

  // ...whereas a default-constructed jdi::definition (JDI1's own parser
  // never touches `cursor`) stays null.
  jdi::definition default_def;
  EXPECT_NE(clang_Cursor_isNull(default_def.cursor), 0);
}

TEST_F(ClangPopulatorTest, TypedefAndEnum) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);

  jdi::definition* typedef_def = global->find_local("demo_int_t");
  ASSERT_NE(typedef_def, nullptr);
  EXPECT_EQ(typedef_def->flags & (jdi::DEF_TYPENAME | jdi::DEF_TYPED),
            (unsigned)(jdi::DEF_TYPENAME | jdi::DEF_TYPED));

  jdi::definition* enum_def = global->find_local("Color");
  ASSERT_NE(enum_def, nullptr);
  EXPECT_EQ(enum_def->flags & (jdi::DEF_ENUM | jdi::DEF_TYPENAME),
            (unsigned)(jdi::DEF_ENUM | jdi::DEF_TYPENAME));

  // Enum constants of an unscoped enum land as siblings in the enclosing
  // scope (the enum cursor itself isn't a DEF_SCOPE -- see cursor_kind_to_flags).
  jdi::definition* red_def = global->find_local("RED");
  ASSERT_NE(red_def, nullptr);
  EXPECT_TRUE(red_def->flags & jdi::DEF_TYPED);
  EXPECT_NE(global->find_local("GREEN"), nullptr);
}

TEST_F(ClangPopulatorTest, GlobalVariable) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);

  jdi::definition* counter_def = global->find_local("global_counter");
  ASSERT_NE(counter_def, nullptr);
  EXPECT_TRUE(counter_def->flags & jdi::DEF_TYPED);
}

TEST_F(ClangPopulatorTest, FunctionOverloads) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);

  jdi::definition* greet_def = global->find_local("greet");
  ASSERT_NE(greet_def, nullptr);
  EXPECT_TRUE(greet_def->flags & jdi::DEF_FUNCTION);

  auto* greet_func = dynamic_cast<jdi::definition_function*>(greet_def);
  ASSERT_NE(greet_func, nullptr);
  ASSERT_EQ(greet_func->overloads.size(), 2u);

  // Each overload's ref_stack holds exactly one RT_FUNCTION node; its
  // paramcount() is the parameter count for that overload (see
  // walk_declarator_expr in parsing/ast.cpp for the same construction idiom).
  std::vector<size_t> param_counts;
  for (auto& entry : greet_func->overloads) {
    ASSERT_FALSE(entry.second->referencers.empty());
    param_counts.push_back(entry.second->referencers.top().paramcount());
  }
  std::sort(param_counts.begin(), param_counts.end());
  ASSERT_EQ(param_counts.size(), 2u);
  EXPECT_EQ(param_counts[0], 1u);
  EXPECT_EQ(param_counts[1], 2u);
}

// C-style varargs mark the overload DEF_VARIADIC; non-variadic overloads
// (greet's) never carry it. Parameter packs are deliberately NOT this flag.
TEST_F(ClangPopulatorTest, CStyleVariadicFlag) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);

  auto* variadic_func =
      dynamic_cast<jdi::definition_function*>(global->find_local("printf_like"));
  ASSERT_NE(variadic_func, nullptr);
  ASSERT_EQ(variadic_func->overloads.size(), 1u);
  EXPECT_TRUE(variadic_func->overloads.begin()->second->flags & jdi::DEF_VARIADIC);

  auto* plain_func =
      dynamic_cast<jdi::definition_function*>(global->find_local("greet"));
  ASSERT_NE(plain_func, nullptr);
  for (auto& entry : plain_func->overloads) {
    EXPECT_FALSE(entry.second->flags & jdi::DEF_VARIADIC);
  }
}

TEST_F(ClangPopulatorTest, ClassTemplate) {
  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);

  jdi::definition* box_def = global->find_local("Box");
  ASSERT_NE(box_def, nullptr);
  EXPECT_TRUE(box_def->flags & jdi::DEF_TEMPLATE);
  EXPECT_EQ(box_def->flags & (jdi::DEF_CLASS | jdi::DEF_SCOPE | jdi::DEF_TYPENAME),
            (unsigned)(jdi::DEF_CLASS | jdi::DEF_SCOPE | jdi::DEF_TYPENAME));

  // T1 scope: DEF_TEMPLATE flag only -- no definition_template/arg_key
  // construction (that's T4). The definition is a plain definition_class.
  EXPECT_EQ(dynamic_cast<jdi::definition_template*>(box_def), nullptr);
  EXPECT_NE(dynamic_cast<jdi::definition_class*>(box_def), nullptr);
}

// One-time manual probe, not part of the default suite (SHELLmain.cpp is a
// full engine translation unit; parsing it is slow and this isn't something
// we want gating every build). Its own test suite (not ClangPopulatorTest --
// gtest forbids mixing TEST_F and TEST under one suite name). Run manually
// with:
//   ./emake-tests --gtest_also_run_disabled_tests --gtest_filter='*FullEngineProbe*'
TEST(ClangPopulatorFullEngineProbe, DISABLED_FullEngineProbe) {
  std::filesystem::path root = EnigmaRoot();
  std::filesystem::path shellmain = root / "ENIGMAsystem" / "SHELL" / "SHELLmain.cpp";
  ASSERT_TRUE(std::filesystem::exists(shellmain)) << shellmain;

  clang_adapter::ClangContext ctx;
  ctx.parse_file(shellmain.string(),
                  {(root / "ENIGMAsystem" / "SHELL").string(), (root / "shared").string()},
                  {"JUST_DEFINE_IT_RUN"});

  jdi::definition_scope* global = ctx.get_global();
  ASSERT_NE(global, nullptr);
  for (const char* name : {"variant", "var", "enigma", "enigma_user"}) {
    jdi::definition* def = global->find_local(name);
    EXPECT_NE(def, nullptr) << "expected " << name << " to be resolvable in the global scope";
    if (def) {
      std::cout << name << " flags: " << jdi::flagnames(def->flags) << std::endl;
    }
  }
}
