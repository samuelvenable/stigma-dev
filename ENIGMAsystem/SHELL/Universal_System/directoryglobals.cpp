/*

 MIT License
 
 Copyright © 2022 Samuel Venable
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
*/

#include <string>

#include "Platforms/General/PFmain.h"
#include "apifilesystem/ghc/filesystem.hpp"

namespace enigma_user {

extern unsigned game_id;

std::string get_working_directory() {
  return ngs::fs::directory_get_current_working();
}

std::string get_program_filename() {
  return ngs::fs::executable_get_filename();
}

std::string get_program_directory() { 
  return ngs::fs::executable_get_directory(); 
}

std::string get_program_pathname() { 
  return ngs::fs::executable_get_pathname(); 
}

bool set_working_directory(std::string dname) {
  return ngs::fs::directory_set_current_working(dname);
}

std::string get_game_save_id() {
  auto add_slash = [](std::string dir) {
    #if defined(_WIN32)
    if (!dir.empty() && *dir.rbegin() != '\\')
    return std::string(dir + "\\");
    #else
    if (!dir.empty() && *dir.rbegin() != '/') 
    return std::string(dir + "/");
    #endif
    return dir;
  };
  std::error_code ec;
  #if defined(_WIN32)
  std::string localappdata = ngs::fs::environment_get_variable("LOCALAPPDATA");
  while (!localappdata.empty() && (*localappdata.rbegin() == '\\' || *localappdata.rbegin() == '/')) 
  { localappdata.pop_back(); } ghc::filesystem::create_directories(localappdata, ec);
  std::string result = add_slash(ngs::fs::environment_get_variable("LOCALAPPDATA")) + 
  add_slash(std::to_string(game_id));
  #else
  ghc::filesystem::create_directories(add_slash(ngs::fs::environment_get_variable("HOME")) + std::string(".config"), ec);
  std::string result = add_slash(ngs::fs::environment_get_variable("HOME")) + 
  std::string(".config/") + add_slash(std::to_string(game_id));
  #endif
  return result;
}

} // namespace enigma_user
