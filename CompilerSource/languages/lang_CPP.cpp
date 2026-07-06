/**
  @file  lang_CPP.cpp
  @brief Implements much of the C++ languages adapter class.

  @section License
    Copyright (C) 2008-2012 Josh Ventura
    This file is a part of the ENIGMA Development Environment.

    ENIGMA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, version 3 of the license or any later version.

    This application and its source code is distributed AS-IS, WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE. See the GNU General Public License for more details.

    You should have recieved a copy of the GNU General Public License along
    with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "settings.h"
#include <ctime>
#include <cstdio>
#include "languages/lang_CPP.h"

#include <set>

string lang_CPP::get_name() { return "C++"; }

void lang_CPP::load_extension_locals() {
  if (!namespace_enigma)
    return (cout << "ERROR! ENIGMA NAMESPACE NOT FOUND. THIS SHOULD NOT HAPPEN IF PARSE SUCCEEDED." << endl, void());

  for (unsigned i = 0; i < parsed_extensions.size(); i++)
  {
    if (parsed_extensions[i].implements == "")
      continue;

    jdi::definition* implements = namespace_enigma->look_up(parsed_extensions[i].implements);

    if (!implements or !(implements->flags & jdi::DEF_SCOPE)) {
      cout << "ERROR! Extension implements " << parsed_extensions[i].implements << " without defining it!" << endl;
      continue;
    }
    if (!(implements->flags & jdi::DEF_CLASS)) {
      cout << "WARNING! Extension implements non-class " << parsed_extensions[i].implements << "!" << endl;
    }
    jdi::definition_scope *const iscope = (jdi::definition_scope*) implements;
    for (jdi::definition_scope::defiter it = iscope->members.begin(); it != iscope->members.end(); ++it) {
      if ((!it->second->flags) & jdi::DEF_TYPED) { cout << "WARNING: Non-scalar `" << it->first << "' ignored." << endl; continue; }
        shared_object_locals_.insert(it->second->name);
    }
  }
}

#ifdef _WIN32
 #define byte __windows_byte_workaround
 #include <windows.h>
 #include "win32_macro_guard.h"
 #undef byte
 #define DLLEXPORT extern "C" __declspec(dllexport)
   #define DECLARE_TIME_TYPE clock_t
   #define CURRENT_TIME(t) t = clock()
   #define PRINT_TIME(ts, te) (((te - ts) * 1000)/CLOCKS_PER_SEC)
#else
 #define DLLEXPORT extern "C"
 #include <cstdio>
 #include <sys/time.h>
   #define DECLARE_TIME_TYPE timeval
   #define CURRENT_TIME(t) gettimeofday(&t,NULL)
   #define PRINT_TIME(ts,te) ((double(te.tv_sec - ts.tv_sec) + double(te.tv_usec - ts.tv_usec)/1000000.0)*1000)
#endif


#include "settings-parse/parse_ide_settings.h"
#include "settings-parse/crawler.h"
#include "languages/clang_adapter.h"
#include "eyaml/eyaml.h"

#include <System/builtins.h>

namespace {

std::string TranscribeTokens(const jdi::token_vector &tokens) {
  std::string result;
  for (const jdi::token_t &token : tokens) {
    if (result.length()) result.push_back(' ');
    result += token.content.toString();
  }
  return result;
}

enigma::parsing::Macro TranslateMacro(const jdi::macro_type &macro,
                                      enigma::parsing::ErrorHandler *herr) {
  using namespace enigma::parsing;
  if (macro.is_function) {
    auto copy = macro.params;
    return enigma::parsing::Macro(
        macro.name, std::move(copy), macro.is_variadic,
        TranscribeTokens(macro.raw_value), herr);
  }
  return enigma::parsing::Macro(
      macro.name, TranscribeTokens(macro.raw_value), herr);
}

}  // namespace

syntax_error *lang_CPP::definitionsModified(const char* wscode,
                                            const char* targetYaml) {
  cout << "Parsing settings..." << endl;
  parse_ide_settings(targetYaml, &compatibility_opts_);

  cout << targetYaml << endl;

  cout << "Creating swap." << endl;
  // Reset cached definition pointers before the context they point into dies.
  namespace_enigma = nullptr;
  namespace_enigma_user = nullptr;
  enigma_type__var = nullptr;
  enigma_type__variant = nullptr;
  enigma_type__varargs = nullptr;
  delete main_context;
  main_context = new clang_adapter::ClangContext();

  cout << "Dumping whiteSpace definitions..." << endl;
  // Definition pages reach the engine parse through the SHELLmain include,
  // at global scope under `using namespace enigma_user`. Clang does not
  // scar its scope stack the way JDI's parser did, so the include route
  // alone suffices -- the old wrap-and-reparse workaround is gone.
  FILE *of = wscode ? fopen((codegen_directory/"Preprocessor_Environment_Editable/IDE_EDIT_whitespace.h").u8string().c_str(),"wb") : NULL;
  if (of) fputs(wscode,of), fclose(of);

  cout << "Opening ENIGMA for parse..." << endl;

  DECLARE_TIME_TYPE ts, te;
  CURRENT_TIME(ts);

  // Include paths mirror what the game Makefile passes: the codegen
  // directory for generated headers, each system's Info directory, the
  // platform-graphics bridges, and the base source roots.
  std::vector<std::string> extra_include_dirs;
  extra_include_dirs.push_back(codegen_directory.u8string());
  std::filesystem::path shell_dir = enigma_root / "ENIGMAsystem" / "SHELL";
  extra_include_dirs.push_back((shell_dir / "Platforms" / extensions::targetAPI.windowSys / "Info").u8string());
  extra_include_dirs.push_back((shell_dir / "Graphics_Systems" / extensions::targetAPI.graphicsSys / "Info").u8string());
  extra_include_dirs.push_back((shell_dir / "Audio_Systems" / extensions::targetAPI.audioSys / "Info").u8string());
  extra_include_dirs.push_back((shell_dir / "Collision_Systems" / extensions::targetAPI.collisionSys / "Info").u8string());
  extra_include_dirs.push_back((shell_dir / "Widget_Systems" / extensions::targetAPI.widgetSys / "Info").u8string());
  extra_include_dirs.push_back((shell_dir / "Networking_Systems" / extensions::targetAPI.networkSys / "Info").u8string());
  extra_include_dirs.push_back((shell_dir / "Universal_System" / "Info").u8string());
  std::filesystem::path platform_graphics_bridge =
      shell_dir / "Bridges" / (extensions::targetAPI.windowSys + "-" + extensions::targetAPI.graphicsSys);
  if (std::filesystem::exists(platform_graphics_bridge)) {
    extra_include_dirs.push_back(platform_graphics_bridge.u8string());
  }
  std::string graphics = extensions::targetAPI.graphicsSys;
  if (graphics.find("OpenGL") != std::string::npos) {
    std::filesystem::path opengl_bridge = shell_dir / "Bridges" / "OpenGL";
    if (std::filesystem::exists(opengl_bridge)) {
      extra_include_dirs.push_back(opengl_bridge.u8string());
    }
    std::filesystem::path opengl_common = shell_dir / "Graphics_Systems" / "OpenGL-Common";
    if (std::filesystem::exists(opengl_common)) {
      extra_include_dirs.push_back(opengl_common.u8string());
    }
  }
  if (graphics.find("OpenGLES") != std::string::npos) {
    std::filesystem::path opengles_bridge = shell_dir / "Bridges" / "OpenGLES";
    if (std::filesystem::exists(opengles_bridge)) {
      extra_include_dirs.push_back(opengles_bridge.u8string());
    }
  }
  extra_include_dirs.push_back(shell_dir.u8string());
  extra_include_dirs.push_back((enigma_root / "shared").u8string());
  if (std::filesystem::exists("/opt/homebrew/include")) {
    extra_include_dirs.push_back("/opt/homebrew/include/");  // macOS Homebrew
  }

  // JUST_DEFINE_IT_RUN no longer hides unparseable code -- clang eats the
  // real headers -- but the guards it drives also fence off per-game
  // generated headers that do not exist at definitions time.
  std::vector<std::string> defines;
  defines.push_back("JUST_DEFINE_IT_RUN");
  {
    ey_data settree = parse_eyaml_str(targetYaml);
    if (settree.get("target-mode").toString() == "Debug") {
      defines.push_back("DEBUG_MODE");
    }
  }

  int res = main_context->parse_file(
      (enigma_root/"ENIGMAsystem/SHELL/SHELLmain.cpp").u8string(),
      extra_include_dirs, defines);
  CURRENT_TIME(te);

  // Diagnose but proceed on a failed parse, as the JDI flow always did:
  // callers (including the test harness, whose sparse settings yield no
  // usable include paths) depend on the fallback scope assignments below
  // even when the engine itself did not resolve.
  if (res != 0) {
    cout << "ERROR: engine parse failed (" << res << "); continuing with "
            "whatever resolved" << endl;
  }

  jdi::definition *d;
  if ((d = main_context->get_global()->look_up("variant"))) {
    enigma_type__variant = d;
    if (!(d->flags & jdi::DEF_TYPENAME))
      cerr << "ERROR! ENIGMA's variant is not a type!" << endl;
    else
      cout << "Successfully loaded builtin variant type" << endl;
  } else cerr << "ERROR! No variant type found!" << endl;
  if ((d = main_context->get_global()->look_up("var"))) {
    enigma_type__var = d;
    if (!(d->flags & jdi::DEF_TYPENAME))
      cerr << "ERROR! ENIGMA's var is not a type!" << endl;
    else
      cout << "Successfully loaded builtin var type" << endl;
  } else cerr << "ERROR! No var type found!" << endl;
  if ((d = main_context->get_global()->look_up("enigma"))) {
    if (d->flags & jdi::DEF_NAMESPACE) {
      namespace_enigma = (jdi::definition_scope*) d;
      if ((d = namespace_enigma->look_up("varargs"))) {
        enigma_type__varargs = d;
        if (!(d->flags & jdi::DEF_TYPENAME))
          cerr << "ERROR! ENIGMA's varargs is not a type!" << endl;
        else
          cout << "Successfully loaded builtin varargs type" << endl;
      } else cerr << "ERROR! No varargs type found!" << endl;
    } else cerr << "ERROR! Namespace enigma is... not a namespace!" << endl;
  } else cerr << "ERROR! Namespace enigma not found!" << endl;
  namespace_enigma_user = main_context->get_global();
  if ((d = main_context->get_global()->look_up("enigma_user"))) {
    if (d->flags & jdi::DEF_NAMESPACE) {
      namespace_enigma_user = (jdi::definition_scope*) d;
    } else cerr << "ERROR! Namespace enigma_user is... not a namespace!" << endl;
  } else cerr << "ERROR! Namespace enigma_user not found!" << endl;
  if (jdi::definition *dstd = main_context->get_global()->look_up("std")) {
    if (dstd->flags & jdi::DEF_NAMESPACE) {
      jdi::definition_scope *j_std = (jdi::definition_scope*) dstd;
      jdi::definition *j_string = j_std->look_up("string");
      if (!j_string) cerr << "Error! std::string was not detected! The parse output probably sucks.";
      else if (!(j_string->flags & jdi::DEF_TYPENAME))
        cerr << "Error! std::string is not a type! The parse output probably sucks.";
      else
        cout << "Successfully parsed std::string, so data is probably good.";
    } else cerr << "ERROR! Namespace enigma_user is... not a namespace!" << endl;
  } else cerr << "ERROR! Namespace std not found!" << endl;

  if (res) {
    cout << "ERROR in parsing engine file: The parser isn't happy. Don't worry, it's never happy.\n";

    ide_passback_error.set(0,0,0,"Parse failed; details in stdout. Bite me.");
    cout << "Continuing anyway." << endl;
    // return &ide_passback_error;
  } else {
    cout << "Successfully parsed ENIGMA's engine (" << PRINT_TIME(ts,te) << "ms)\n"
    << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    //cout << "Namespace std contains " << global_scope.members["std"]->members.size() << " items.\n";
  }

  cout << "Creating dummy primitives for old ENIGMA" << endl;
  for (jdi::tf_iter it = jdi::builtin_declarators.begin(); it != jdi::builtin_declarators.end(); ++it) {
    main_context->get_global()->members[it->first] = std::make_unique<jdi::definition>(it->first, main_context->get_global(), jdi::DEF_TYPENAME);
  }

  enigma::parsing::StdErrorHandler hack;  // TODO: FIXME: This should be using a central error handler...
  cout << "Translating macros to EDL...\n";
  // The syntax-quirk lowering macros (syntax_quirks.h) exist so EMITTED C++
  // can carry EDL's operator keywords verbatim and lower at C++ compile time.
  // They must not expand in EDL source: there, `repeat`/`until` are statement
  // productions and `mod`/`div` are operator keywords (TT_MOD/TT_DIV), and a
  // macro expansion would hijack them before the keyword table sees the name.
  static const std::set<std::string_view> kEmissionOnlyMacros{
      "repeat", "until", "mod", "div"};
  // ClangContext hands back our own Macro type directly; no translation.
  for (auto &macro_pair : main_context->get_macros()) {
    if (kEmissionOnlyMacros.count(macro_pair.first)) continue;
    builtin_macros_.insert({macro_pair.first, std::move(*macro_pair.second)});
  }

  cout << "Grabbing locals...\n";

  load_shared_locals();  // Extensions were separated above

  cout << "Determining build target...\n";

  extensions::determine_target();

  cout << " Done.\n";

  return &ide_passback_error;
}

#include "compiler/compile_common.h"

int lang_CPP::load_shared_locals() {
  cout << "Finding parent..."; fflush(stdout);

  // Find namespace enigma
  jdi::definition* pscope = main_context->get_global()->look_up("enigma");
  if (!pscope or !(pscope->flags & jdi::DEF_SCOPE)) {
    cerr << "ERROR! Can't find namespace enigma!" << endl;
    return 1;
  }
  jdi::definition_scope* ns_enigma = (jdi::definition_scope*)pscope;
  jdi::definition* parent = ns_enigma->look_up(system_get_uppermost_tier());
    if (!parent) {
      cerr << "ERROR! Failed to find parent scope `" << system_get_uppermost_tier() << endl;
      return 2;
    }
  if (not(parent->flags & jdi::DEF_CLASS)) {
    cerr << "PARSE ERROR! Parent class is not a class?" << endl;
    cout << parent->parent->name << "::" << parent->name << ":  " << parent->toString() << endl;
    return 3;
  }
  jdi::definition_class *pclass = (jdi::definition_class*)parent;

  // Find the parent object
  cout << "Found parent scope" << endl;

  shared_object_locals_.clear();

  //Iterate the tiers of the parent object
  for (jdi::definition_class *cs = pclass; cs; cs = (cs->ancestors.size() ? cs->ancestors[0].def : NULL) )
  {
    cout << " >> Checking ancestor " << cs->name << endl;
    for (jdi::definition_scope::defiter mem = cs->members.begin(); mem != cs->members.end(); ++mem)
      shared_object_locals_.insert(mem->first);
  }

  load_extension_locals();
  return 0;
}

// The global-scope fallback exposes engine and definition-page globals,
// never system headers: the C library also lives at global scope, and a
// GML variable named j0 must shadow the Bessel function, not assign to
// it. This is the surface JDI1 exposed by accident -- its parse died in
// glibc before the C library could pollute the scope. The pocketed
// cursor says which file declared the definition.
static bool is_edl_visible_global(jdi::definition *d) {
  if (clang_Cursor_isNull(d->cursor)) return false;
  CXFile file;
  unsigned line, col, off;
  clang_getExpansionLocation(clang_getCursorLocation(d->cursor),
                             &file, &line, &col, &off);
  if (!file) return false;
  CXString fname = clang_getFileName(file);
  const char *s = clang_getCString(fname);
  bool visible = false;
  if (s) {
    std::string_view path(s);
    visible = path.find("IDE_EDIT_whitespace.h") != std::string_view::npos ||
              path.rfind(enigma_root.u8string(), 0) == 0;
  }
  clang_disposeString(fname);
  return visible;
}

jdi::definition* lang_CPP::look_up(std::string_view n) const {
  // TODO: FIXME: slow-ass conversion still exists...
  std::string name(n);
  auto builtin = jdi::builtin_declarators.find(name);
  if (builtin != jdi::builtin_declarators.end()) return builtin->second->def;
  if (namespace_enigma_user) {
    if (jdi::definition *d = namespace_enigma_user->find_local(name)) return d;
  }
  // Definition pages land at global scope (SHELLmain includes them under
  // `using namespace enigma_user`), so user definitions resolve here. JDI1
  // reached them by accident: its engine parse usually failed to resolve
  // enigma_user at all, defaulting this lookup to the global scope anyway.
  if (main_context && main_context->get_global() &&
      main_context->get_global() != namespace_enigma_user) {
    jdi::definition *d = main_context->get_global()->find_local(name);
    if (d && is_edl_visible_global(d)) return d;
  }
  return nullptr;
}

// TODO: This could use better plumbing.
lang_CPP::lang_CPP(): evdata_(ParseEventFile((enigma_root/"events.ey").u8string())) {}
