#ifndef PARSER_TESTING_UTILS
#define PARSER_TESTING_UTILS

#include <gtest/gtest.h>
#include <languages/lang_CPP.h>
#include <parsing/ast.h>
#include <parsing/parser.h>
#include "../../../CompilerSource/OS_Switchboard.h"
#include "matchers.h"

using namespace ::enigma::parsing;

static const NameSet kNoNames;

class TestFailureErrorHandler : public ErrorHandler {
 public:
  void ReportError(CodeSnippet snippet, std::string_view error) final {
    ADD_FAILURE() << "Test reported an error at line " << snippet.line << ", position " << snippet.position << ": "
                  << error;
  }
  void ReportWarning(CodeSnippet snippet, std::string_view warning) final {
    ADD_FAILURE() << "Test reported a warning at line " << snippet.line << ", position " << snippet.position << ": "
                  << warning;
  }
};

struct ParserTester {
  TestFailureErrorHandler herr;
  const ParseContext* context;
  Lexer lexer;
  lang_CPP cpp{};
  AstBuilderTestAPI* builder = CreateBuilder();

  AstBuilderTestAPI* operator->() { return builder; }

  explicit ParserTester(std::string code, bool use_cpp)
      : context(&ParseContext::ForTesting(use_cpp)), lexer(std::move(code), context, &herr) {
    builder->initialize(&lexer, &herr);
  }

  explicit ParserTester(std::string code) : context(&SetUp()), lexer(std::move(code), context, &herr) {
    builder->initialize(&lexer, &herr);
  }

  // Settings-aware constructor: identical to the SetUp-based one, but appends
  // `extra_yaml` to the base YAML config so individual tests can override
  // compatibility flags (e.g. "inherit-increment-from: 1\n"). Re-runs
  // definitionsModified on every call; settings live as long as the shared
  // static lang_CPP does, so later tests should reset what they care about.
  ParserTester(std::string code, std::string_view extra_yaml, bool /*tag*/)
      : context(&SetUpWithSettings(extra_yaml)), lexer(std::move(code), context, &herr) {
    builder->initialize(&lexer, &herr);
  }

  static ParserTester CreateWithCpp(std::string code) { return ParserTester(std::move(code), true); }

  static ParserTester CreateWithoutCpp(std::string code) { return ParserTester(std::move(code), false); }

  static ParserTester CreateWithSetUp(std::string code) { return ParserTester(std::move(code)); }

  static ParserTester CreateWithSettings(std::string code, std::string_view extra_yaml) {
    return ParserTester(std::move(code), extra_yaml, /*tag=*/true);
  }

  const ParseContext& SetUp() {
    static lang_CPP cpp{};
    static bool initialized = false;

    if (!initialized) {
      cpp.definitionsModified(NULL, ((string) "%e-yaml\n"
      "---\n"
      "target-windowing: " +  (CURRENT_PLATFORM_ID==OS_WINDOWS ? "Win32" : CURRENT_PLATFORM_ID==OS_MACOSX ? "Cocoa" : "xlib")  + "\n"
      "treat-literals-as: 0\n"
      "sample-lots-of-radios: 0\n"
      "inherit-equivalence-from: 0\n"
      "sample-checkbox: on\n"
      "sample-edit: DEADBEEF\n"
      "sample-combobox: 0\n"
      "inherit-strings-from: 0\n"
      "inherit-escapes-from: 0\n"
      "inherit-increment-from: 0\n"
      " \n"
      "target-audio: OpenAL\n"
      "target-windowing: xlib\n"
      "target-compiler: gcc\n"
      "target-graphics: OpenGL\n"
      "target-widget: None\n"
      "target-collision: BBox\n"
      "target-networking: None\n"
      ).c_str());

      initialized = true;
    }
    static ParseContext context(&cpp, kNoNames);
    return context;
  }

  const ParseContext& SetUpWithSettings(std::string_view extra_yaml) {
    // Ensure baseline static cpp + ParseContext exist.
    const ParseContext &base_ctx = SetUp();
    // Mutate the existing static lang_CPP (reached via the base context) with
    // the overridden YAML so subsequent parses pick up the extra settings.
    auto *cpp = const_cast<lang_CPP*>(static_cast<const lang_CPP*>(base_ctx.language_fe));
    std::string yaml = (string) "%e-yaml\n"
      "---\n"
      "target-windowing: " +  (CURRENT_PLATFORM_ID==OS_WINDOWS ? "Win32" : CURRENT_PLATFORM_ID==OS_MACOSX ? "Cocoa" : "xlib")  + "\n"
      "treat-literals-as: 0\n"
      "sample-lots-of-radios: 0\n"
      "inherit-equivalence-from: 0\n"
      "sample-checkbox: on\n"
      "sample-edit: DEADBEEF\n"
      "sample-combobox: 0\n"
      "inherit-strings-from: 0\n"
      "inherit-escapes-from: 0\n"
      "inherit-increment-from: 0\n"
      " \n"
      "target-audio: OpenAL\n"
      "target-windowing: xlib\n"
      "target-compiler: gcc\n"
      "target-graphics: OpenGL\n"
      "target-widget: None\n"
      "target-collision: BBox\n"
      "target-networking: None\n";
    yaml += std::string(extra_yaml);
    cpp->definitionsModified(NULL, yaml.c_str());
    return base_ctx;
  }

  void TearDown() {
    delete main_context;
    delete builder;
    delete current_language;
    jdi::clean_up();
  }
};

#endif
