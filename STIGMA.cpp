#include <algorithm>
#include <thread>
#include <chrono>
#include <string>

#include "shared/apifilesystem/filesystem.hpp"
#include "shared/apiprocess/process.hpp"

int main() {
  apiprocess::proc_id_t pid = 0;
  ngs::fs::directory_set_current_working(ngs::fs::executable_get_directory());
  std::string InstDir = ngs::fs::filename_absolute("..\\");
  if (InstDir.empty()) return 0;
  std::string dir = ngs::fs::executable_get_directory();
  std::replace(dir.begin(), dir.end(), '\\', '/');
  if (ngs::fs::file_exists(ngs::fs::executable_get_directory() + "LateralGM-Windows-i686.exe")) {
    ngs::fs::environment_set_variable("PATH", (dir + ";" + InstDir + "msys64;" + InstDir + "msys64\\mingw32\\bin;" + InstDir + "msys64\\usr\\local\\bin;" + InstDir + "msys64\\usr\\bin;%PATH%").c_str());
    pid = apiprocess::spawn_child_proc_id("\"" + dir + "LateralGM-Windows-i686.exe\"", false);
  } else if (ngs::fs::file_exists(ngs::fs::executable_get_directory() + "LateralGM-Windows-x86_64.exe")) {
    ngs::fs::environment_set_variable("PATH", (dir + ";" + InstDir + "msys64;" + InstDir + "msys64\\mingw64\\bin;" + InstDir + "msys64\\usr\\local\\bin;" + InstDir + "msys64\\usr\\bin;%PATH%").c_str());
    pid = apiprocess::spawn_child_proc_id("\"" + dir + "LateralGM-Windows-x86_64.exe\"", false);
  }
  if (pid) {
    while (!apiprocess::child_proc_id_is_complete(pid))
      std::this_thread::sleep_for(std::chrono::seconds(1));
    apiprocess::free_stdout_for_child_proc_id(pid);
    apiprocess::free_stdin_for_child_proc_id(pid);
  }
  return 0;
}
