/*

 MIT License
 
 Copyright © 2023 Samuel Venable
 
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

#if (defined(_WIN32) && defined(_MSC_VER))
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <regex>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#if (!defined(_WIN32) && (!defined(__APPLE__) && !defined(__MACH__)))
#if defined(CREATE_CONTEXT)
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif
#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <dxgi.h>
#else
#if defined(__APPLE__)
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_time.h>
#include "shmem.hpp"
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif
#if ((defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__sun))
#include <sys/types.h>
#if (defined(__FreeBSD__) || defined(__DragonFly__))
#include <fcntl.h>
#include <kvm.h>
#elif (defined(__NetBSD__) || defined(__OpenBSD__))
#if defined(__NetBSD__)
#include <uvm/uvm_extern.h>
#endif
#include <sys/param.h>
#include <sys/swap.h>
#endif
#endif
#if !defined(__sun)
#if !defined(__linux__)
#include <sys/sysctl.h>
#endif
#include <sys/utsname.h>
#else
#include <sys/systeminfo.h>
#include <sys/swap.h>
#endif
#include <unistd.h>
#endif
#if (defined(__DragonFly__) || defined(__NetBSD__) || defined(__sun))
#include <hwloc.h>
#endif
#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "dxgi.lib")
#endif

#include "pci.ids.hpp"
#include "system.hpp"

namespace ngs::sys {

namespace {

#if (!defined(_WIN32) && (!defined(__APPLE__) && !defined(__MACH__)))
#if defined(CREATE_CONTEXT)
SDL_Window *window = nullptr;
#endif
#endif

#if defined(_WIN32)
std::string windows_product_name;
std::string windows_version_number;
#endif

std::string devicename;
bool devicenameerror = false;

std::string productname;
bool productnameerror = false;

std::string kernelname;
bool kernelnameerror = false;

std::string kernelrelease;
bool kernelreleaseerror = false;

std::string kernelversion;
bool kernelversionerror = false;

std::string architecture;
bool architectureerror = false;

unsigned device_id = 0;
std::unordered_map<unsigned, std::string> vendor_name_by_id;

unsigned vendor_id = 0;
std::unordered_map<unsigned, std::string> device_name_by_id;

std::string gpuvendor;
bool gpuvendorerror = false;

std::string gpurenderer;
bool gpurenderererror = false;

long long videomemory = -1;
bool videomemoryerror = false;

std::string cpuvendor;
bool cpuvendorerror = false;

std::string cpubrand;
bool cpubranderror = false;

int numcores = -1;
bool numcoreserror = false;

int numcpus = -1;
bool numcpuserror = false;

long long totalram = -1;
bool totalramerror = false;

std::string isvirtual;
bool isvirtualerror = false;

#if defined(_WIN32)
std::string wine_version;
#endif

struct hreadable {
  long double size = 0;
private: friend
  std::ostream& operator<<(std::ostream& os, hreadable hr) {
    int i = 0;
    long double mantissa = hr.size;
    for (; mantissa >= 1024; mantissa /= 1024, i++) { }
    mantissa = std::ceil(mantissa * 100) / 100;
    os << mantissa << " " << "BKMGTPE"[i];
    return !i ? os : os << "B";
  }
};

std::string pointer_null() {
  return "(null)";
}

std::string make_hreadable(long double nbytes) {
  if (nbytes == -1) 
    return pointer_null();
  std::stringstream ss;
  ss << hreadable{nbytes};
  return ss.str();
}

/* Define CREATE_CONTEXT in your build scripts or Makefiles if
the calling process hasn't already done this on its own ... */
#if (!defined(_WIN32) && (!defined(__APPLE__) && !defined(__MACH__)))
#if defined(CREATE_CONTEXT)
bool create_context() {
  if (!window) {
    SDL_setenv("SDL_VIDEODRIVER", "x11", 1);
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    if (SDL_Init(SDL_INIT_VIDEO))
      return false;
    window = SDL_CreateWindow("", 0, 0, 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) 
      return false;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) 
      return false;
    int err = SDL_GL_MakeCurrent(window, context);
    if (err) 
      return false;
  }
  return true;
}
#endif
#endif

#if defined(_WIN32)
void message_pump() {
  MSG msg;
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

std::string wine_get_version() {
  if (!wine_version.empty())
    return wine_version;
  static const char *(CDECL *pwine_get_version)(void);
  HMODULE hntdll = GetModuleHandleW(L"ntdll.dll");
  if (!hntdll)
    return pointer_null();
  pwine_get_version = (const char *(*)())GetProcAddress(hntdll, "wine_get_version");
  if (!pwine_get_version)
    return pointer_null();
  return pwine_get_version();
}
#endif

std::string read_output(std::string cmd) {
  std::string result;
  #if defined(_WIN32)
  bool proceed = true;
  HANDLE stdin_read = nullptr;
  HANDLE stdin_write = nullptr;
  HANDLE stdout_read = nullptr; 
  HANDLE stdout_write = nullptr;
  SECURITY_ATTRIBUTES sa = { 
    sizeof(SECURITY_ATTRIBUTES), 
    nullptr, 
    true 
  };
  proceed = CreatePipe(&stdin_read, &stdin_write, &sa, 0);
  if (!proceed)
    return "";
  SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);
  proceed = CreatePipe(&stdout_read, &stdout_write, &sa, 0);
  if (!proceed)
    return "";
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  si.hStdOutput = stdout_write;
  si.hStdInput = stdin_read;
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  std::vector<char> ccmd;
  ccmd.resize(cmd.length() + 1, '\0');
  strncpy_s(&ccmd[0], cmd.length() + 1, cmd.c_str(), cmd.length() + 1);
  BOOL success = CreateProcessA(nullptr, &ccmd[0], nullptr, nullptr, true, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  if (success) {
    DWORD nRead = 0;
    char buffer[1024];
    CloseHandle(stdout_write);
    CloseHandle(stdin_read);
    HANDLE wait_handles[] = { 
      pi.hProcess,
      stdout_read
    };
    while (MsgWaitForMultipleObjects(2, wait_handles, false, 5, QS_ALLEVENTS) != WAIT_OBJECT_0) {
      message_pump();
      while (ReadFile(stdout_read, buffer, 1024, &nRead, nullptr) && nRead) {
        message_pump();
        buffer[nRead] = '\0';
        result.append(buffer, nRead);
      }
      // remove trailing whitespace and newlines we do not need in return strings
      while (!result.empty() && (result.back() == ' ' || result.back() == '\t' ||
        result.back() == '\r' || result.back() == '\n')) {
        message_pump();
        result.pop_back();
      }
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(stdout_read);
    CloseHandle(stdin_write);
  }
  #else
  char buf[1024];
  ssize_t nRead = 0;
  FILE *fp = popen(cmd.c_str(), "r");
  if (fp) {
    while ((nRead = read(fileno(fp), buf, 1024)) > 0) {
      buf[nRead] = '\0';
      result.append(buf, nRead);
    }
    // remove trailing whitespace and newlines we do not need in return strings
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t' ||
      result.back() == '\r' || result.back() == '\n'))
      result.pop_back();
    pclose(fp);
  }
  #endif
  static std::string str;
  str = result;
  return str;
}

std::vector<std::string> string_split(std::string str, char delimiter) {
  std::vector<std::string> vec;
  std::stringstream sstr(str);
  std::string tmp;
  while (std::getline(sstr, tmp, delimiter)) {
    #if defined(_WIN32)
    message_pump();
    #endif
    vec.push_back(tmp);
  }
  return vec;
}

#if defined(__linux__)
long long read_meminfo(std::string key) {
  long long meminfo = -1;
  std::fstream doc;
  doc.open("/proc/meminfo", std::ios::in);
  if (doc.is_open()) {
    std::string tmp;
    while (std::getline(doc, tmp)) {
      if (tmp.substr(0, key.length()) == key) {
        key = std::regex_replace(key, std::regex(":"), "");
        tmp = std::regex_replace(tmp, std::regex(":"), "");
        tmp = std::regex_replace(tmp, std::regex(" kB"), "");
        tmp = std::regex_replace(tmp, std::regex(" "), "");
        meminfo = strtoll(tmp.substr(key.length()).c_str(), nullptr, 10) * 1024;
        break;
      }
    }
    doc.close();
  }
  return meminfo;
}
#endif

#if defined(_WIN32)
void allocate_windows_version_number_and_product_name() {
  auto GetOSMajorVersionNumber = []() {
    int val = 0;
    char buf[1024];
    static std::string str;
    DWORD sz = sizeof(val);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\", "CurrentMajorVersionNumber", RRF_RT_REG_DWORD, nullptr, &val, &sz) == ERROR_SUCCESS) {
      if (sprintf(buf, "%d", val) != -1) {
        str = buf;
      }
    }
    return str;
  };
  auto GetOSMinorVersionNumber = []() {
    int val = 0;
    char buf[1024];
    static std::string str;
    DWORD sz = sizeof(val);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\", "CurrentMinorVersionNumber", RRF_RT_REG_DWORD, nullptr, &val, &sz) == ERROR_SUCCESS) {
      if (sprintf(buf, "%d", val) != -1) {
        str = buf;
      }
    }
    return str;
  };
  auto GetOSBuildNumber = []() {
    char buf[1024];
    static std::string str;
    DWORD sz = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\", "CurrentBuildNumber", RRF_RT_REG_SZ, nullptr, &buf, &sz) == ERROR_SUCCESS) {
      str = buf;
    }
    return str;
  };
  auto GetOSRevisionNumber = []() {
    int val = 0;
    char buf[1024];
    static std::string str;
    DWORD sz = sizeof(val);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\", "UBR", RRF_RT_REG_DWORD, nullptr, &val, &sz) == ERROR_SUCCESS) {
      if (sprintf(buf, "%d", val) != -1) {
        str = buf;
      }
    }
    return str;
  };
  char buf[1024];
  if (!GetOSMajorVersionNumber().empty() && !GetOSMinorVersionNumber().empty() && !GetOSBuildNumber().empty() && !GetOSRevisionNumber().empty()) {
    if (sprintf(buf, "%s.%s.%s.%s", GetOSMajorVersionNumber().c_str(), GetOSMinorVersionNumber().c_str(), GetOSBuildNumber().c_str(), GetOSRevisionNumber().c_str()) != -1) {
      windows_version_number = buf;
      char buf[1024];
      DWORD sz = sizeof(buf);
      if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\", "ProductName", RRF_RT_REG_SZ, nullptr, &buf, &sz) == ERROR_SUCCESS) {
        if (strtoull(GetOSMajorVersionNumber().c_str(), nullptr, 10) == 10 && strtoull(GetOSBuildNumber().c_str(), nullptr, 10) >= 22000) {
          std::string tmp = buf;
          if (!tmp.empty()) {
            tmp = std::regex_replace(tmp, std::regex("10"), "11");
            windows_product_name = tmp;
          }
        } else {
          windows_product_name = buf;
        }
      }
    }
  }
}
#endif

std::string get_vendor_or_device_name_by_id(unsigned identifier, bool vendor_or_device) {
  if (vendor_name_by_id.find(identifier) != vendor_name_by_id.end() && !vendor_or_device)
    return vendor_name_by_id[identifier];
  if (device_name_by_id.find(identifier) != device_name_by_id.end() && vendor_or_device)
    return device_name_by_id[identifier];
  std::string str(pci_ids, pci_ids + sizeof(pci_ids) / sizeof(pci_ids[0]));
  str = std::regex_replace(str, std::regex("\r"), "");
  std::vector<std::string> vec = string_split(str, '\n');
  for (std::size_t i = 0; i < vec.size(); i++) {
    #if defined(_WIN32)
    message_pump();
    #endif
    if (vec[i].empty() || (!vec[i].empty() && vec[i][0] == '#'))
      continue;
    std::size_t pos1 = vec[i].find(" ");
    if (pos1 != std::string::npos && vec[i][0] != '\t') {
      std::istringstream converter1(vec[i].substr(0, pos1));
      converter1 >> std::hex >> vendor_id;
      vendor_name_by_id[vendor_id] = vec[i].substr(pos1 + 2);
    }
    if (vendor_name_by_id[vendor_id] != vendor_name_by_id[identifier])
      continue;
    std::size_t pos2 = vec[i].find("\t\t");
    if (pos2 == std::string::npos && vec[i][0] == '\t') {
      std::size_t pos3 = vec[i].find(" ");
      if (pos3 != std::string::npos) {
        std::istringstream converter2(vec[i].substr(1, 4));
        converter2 >> std::hex >> device_id;
        device_name_by_id[device_id] = &vec[i].substr(pos3)[2];
      }
    }
  }
  return vendor_or_device ? device_name_by_id[identifier] : vendor_name_by_id[identifier];
}

} // anonymous namespace

std::string os_kernel_name() {
  if (!kernelname.empty())
    return kernelname;
  if (kernelnameerror)
    return pointer_null();
  #if !defined(_WIN32)
  #if !defined(__sun)
  struct utsname name;
  if (!uname(&name))
    kernelname = name.sysname;
  #else
  long count = sysinfo(SI_SYSNAME, nullptr, 0);
  if (count > 0) {
    char *buf = (char *)calloc(count, sizeof(char));
    if (buf) {
      if (sysinfo(SI_SYSNAME, buf, count) > 0)
        kernelname = buf;
      free(buf);
    }
  }
  #endif
  #else
  char buf[1024];
  DWORD sz = sizeof(buf);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment\\", "OS", RRF_RT_REG_SZ, nullptr, &buf, &sz) == ERROR_SUCCESS)
    kernelname = buf;
  #endif
  if (!kernelname.empty())
    return kernelname;
  kernelnameerror = true;
  return pointer_null();
}

std::string os_device_name() {
  if (!devicename.empty())
    return devicename;
  if (devicenameerror)
    return pointer_null();
  #if !defined(_WIN32)
  #if !defined(__sun)
  struct utsname name;
  if (!uname(&name))
    devicename = name.nodename;
  #else
  long count = sysinfo(SI_HOSTNAME, nullptr, 0);
  if (count > 0) {
    char *buf = (char *)calloc(count, sizeof(char));
    if (buf) {
      if (sysinfo(SI_HOSTNAME, buf, count) > 0)
        devicename = buf;
      free(buf);
    }
  }
  #endif
  #else
  char buf[1024];
  WSADATA data;
  WORD wVersionRequested;
  wVersionRequested = MAKEWORD(2, 2);
  if (!WSAStartup(wVersionRequested, &data)) {
    if (!gethostname(buf, sizeof(buf))) {
      devicename = buf;
    }
    WSACleanup();
  }
  #endif
  if (!devicename.empty())
    return devicename;
  devicenameerror = true;
  return pointer_null();
}

std::string os_kernel_release() {
  if (!kernelrelease.empty())
    return kernelrelease;
  if (kernelreleaseerror)
    return pointer_null();
  #if !defined(_WIN32)
  #if !defined(__sun)
  struct utsname name;
  if (!uname(&name))
    kernelrelease = name.release;
  #else
  long count = sysinfo(SI_RELEASE, nullptr, 0);
  if (count > 0) {
    char *buf = (char *)calloc(count, sizeof(char));
    if (buf) {
      if (sysinfo(SI_RELEASE, buf, count) > 0)
        kernelrelease = buf;
      free(buf);
    }
  }
  #endif
  #else
  if (!windows_version_number.empty()) {
    kernelrelease = windows_version_number;
    return kernelrelease;
  }
  allocate_windows_version_number_and_product_name();
  kernelrelease = windows_version_number;
  wine_version = wine_get_version();
  if (wine_version != pointer_null()) {
    std::string tmp = os_kernel_version();
    if (!tmp.empty()) {
      tmp = std::regex_replace(tmp, std::regex("Microsoft Windows "), "");
      tmp = std::regex_replace(tmp, std::regex("v"), "");
      tmp = std::regex_replace(tmp, std::regex("V"), "");
      tmp = std::regex_replace(tmp, std::regex("\\["), "");
      tmp = std::regex_replace(tmp, std::regex("\\]"), "");
      kernelrelease = tmp;
    }
  }
  #endif
  if (!kernelrelease.empty())
    return kernelrelease;
  kernelreleaseerror = true;
  return pointer_null();
}

std::string os_kernel_version() {
  if (!kernelversion.empty())
    return kernelversion;
  if (kernelversionerror)
    return pointer_null();
  #if !defined(_WIN32)
  #if !defined(__sun)
  #if !defined(__DragonFly__)
  struct utsname name;
  if (!uname(&name))
    kernelversion = name.version;
  #else
  /* dragonfly bsd truncates utsname.version;
  get the value from the uname cmd instead */
  kernelversion = read_output("uname -v");
  #endif
  #else
  long count = sysinfo(SI_VERSION, nullptr, 0);
  if (count > 0) {
    char *buf = (char *)calloc(count, sizeof(char));
    if (buf) {
      if (sysinfo(SI_VERSION, buf, count) > 0)
        kernelversion = buf;
      free(buf);
    }
  }
  #endif
  #else
  std::string tmp = read_output("C:\\Windows\\System32\\cmd.exe /c ver");
  if (!tmp.empty()) {
    tmp = std::regex_replace(tmp, std::regex("\t"), "");
    tmp = std::regex_replace(tmp, std::regex("\r"), "");
    tmp = std::regex_replace(tmp, std::regex("\n"), "");
    kernelversion = tmp;
  }
  #endif
  if (!kernelversion.empty())
    return kernelversion;
  kernelversionerror = true;
  return pointer_null();
}

std::string os_product_name() {
  if (!productname.empty())
    return productname;
  if (productnameerror)
    return pointer_null();
  #if defined(_WIN32)
  if (!windows_product_name.empty()) {
    productname = windows_product_name;
    return productname;
  }
  allocate_windows_version_number_and_product_name();
  productname = windows_product_name;
  wine_version = wine_get_version();
  if (wine_version != pointer_null()) {
    productname = "wine-" + wine_version;
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  std::string tmp1 = read_output("echo `sw_vers | grep 'ProductName:' | uniq | awk 'NR==1{$1=$1;print}' && sw_vers | grep 'ProductVersion:' | uniq | awk 'NR==1{$1=$1;print}'`");
  if (!tmp1.empty()) {
    tmp1 = std::regex_replace(tmp1, std::regex("ProductName: "), "");
    tmp1 = std::regex_replace(tmp1, std::regex("ProductVersion: "), "");
    std::fstream doc;
    doc.open("/System/Library/CoreServices/Setup Assistant.app/Contents/Resources/en.lproj/OSXSoftwareLicense.rtf", std::ios::in);
    if (doc.is_open()) {
      std::string tmp2;
      while (std::getline(doc, tmp2)) {
        std::string tmp3 = tmp2;
        std::transform(tmp2.begin(), tmp2.end(), tmp2.begin(), ::toupper);
        std::size_t pos1 = tmp2.find("SOFTWARE LICENSE AGREEMENT FOR MAC OS X");
        std::size_t pos2 = tmp2.find("SOFTWARE LICENSE AGREEMENT FOR MACOS");
        std::size_t len1 = strlen("SOFTWARE LICENSE AGREEMENT FOR MAC OS X");
        std::size_t len2 = strlen("SOFTWARE LICENSE AGREEMENT FOR MACOS");
        if (pos1 != std::string::npos) {
          tmp1 += tmp3.substr(pos1 + len1);
          tmp1 = tmp1.substr(0, tmp1.length() - 1);
          break;
        } else if (pos2 != std::string::npos) {
          tmp1 += tmp3.substr(pos2 + len2);
          tmp1 = tmp1.substr(0, tmp1.length() - 1);
          break;
        }
      }
      doc.close();
    }
    productname = tmp1;
  }
  #elif defined(__linux__)
  std::string tmp = read_output("echo `lsb_release --id 2> /dev/null && lsb_release --release 2> /dev/null && lsb_release --codename 2> /dev/null`");
  if (!tmp.empty()) {
    tmp = std::regex_replace(tmp, std::regex("\r"), "");
    tmp = std::regex_replace(tmp, std::regex("\n"), "");
    tmp = std::regex_replace(tmp, std::regex("Distributor ID: "), "");
    tmp = std::regex_replace(tmp, std::regex("Release: "), "");
    tmp = std::regex_replace(tmp, std::regex("Codename: "), "");
    productname = tmp;
  } else {
    /* if lsb_release is not installed use kernel + release: */
    productname = os_kernel_name() + " " + os_kernel_release();
  }
  #elif !defined(__sun)
  productname = os_kernel_name() + " " + os_kernel_release();
  #else
  productname = read_output("awk 'NR==1{print $1,$2,$3}' /etc/release");
  #endif
  if (!productname.empty())
    return productname;
  productnameerror = true;
  return pointer_null();
}

std::string os_architecture() {
  if (!architecture.empty())
    return architecture;
  if (architectureerror)
    return pointer_null();
  #if !defined(_WIN32)
  #if !defined(__sun)
  /* utsname.machine equals the achitecture of the 
  current executable - not the current platform */
  #if (defined(__APPLE__) && defined(__MACH__))
  /* macOS requires "arch -arch arm64" to 
  force running without rosetta if x86 */
  if (cpu_processor().substr(0, 5) == "Apple") {
    architecture = read_output("arch -arch arm64 uname -m 2> /dev/null");
  } else {
    architecture = read_output("uname -m");
  }
  #else
  architecture = read_output("uname -m");
  #endif
  #else
  long count = sysinfo(SI_ARCHITECTURE_K, nullptr, 0);
  if (count > 0) {
    char *buf = (char *)calloc(count, sizeof(char));
    if (buf) {
      if (sysinfo(SI_ARCHITECTURE_K, buf, count) > 0)
        architecture = buf;
      free(buf);
    }
  }
  #endif
  #else
  char buf[1024];
  DWORD sz = sizeof(buf);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment\\", "PROCESSOR_ARCHITECTURE", RRF_RT_REG_SZ, nullptr, &buf, &sz) == ERROR_SUCCESS)
    architecture = buf;
  #endif
  if (!architecture.empty())
    return architecture;
  architectureerror = true;
  return pointer_null();
}

std::string os_is_virtual() {
  if (!isvirtual.empty())
    return isvirtual;
  if (isvirtualerror)
    return pointer_null();
  #if (defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86))
  if (os_product_name().substr(0, 4) == "wine") {
    isvirtual = "YES";
    return isvirtual;
  }
  #if (defined(__APPLE__) && defined(__MACH__))
  if (read_output("sysctl -in sysctl.proc_translated") == "1") {
    isvirtual = "YES";
    return isvirtual;
  }
  #endif
  class cpuid {
    #if defined(_MSC_VER)
    int regs[4];
    #else
    unsigned regs[4];
    #endif
  public:
    explicit cpuid(unsigned func_id, unsigned sub_func_id) {
      #if defined(_MSC_VER)
      __cpuidex(regs, func_id, sub_func_id);
      #else
      asm volatile ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (func_id), "c" (sub_func_id));
      #endif  
    }
    #if defined(_MSC_VER)
    const int &eax() const {
      return regs[0];
    }
    const int &ebx() const {
      return regs[1];
    }
    const int &ecx() const {
      return regs[2];
    }
    const int &edx() const {
      return regs[3];
    }
    #else
    const unsigned &eax() const {
      return regs[0];
    }
    const unsigned &ebx() const {
      return regs[1];
    }
    const unsigned &ecx() const {
      return regs[2];
    }
    const unsigned &edx() const {
      return regs[3];
    }
    #endif
  };
  if (!(cpuid(1, 0).ecx() & (1 << 31))) {
    isvirtual = "NO";
    return isvirtual;
  }
  const int vendorIdLength = 13;
  using VendorIdStr = char[vendorIdLength];
  VendorIdStr hyperVendorId;
  memcpy(hyperVendorId + 0, &cpuid(0x40000000, 0).ebx(), 4);
  memcpy(hyperVendorId + 4, &cpuid(0x40000000, 0).ecx(), 4);
  memcpy(hyperVendorId + 8, &cpuid(0x40000000, 0).edx(), 4);
  hyperVendorId[12] = '\0';
  static const VendorIdStr vendors[] {
    "VBoxVBoxVBox",      // VirtualBox 
    "bhyve bhyve ",      // bhyve
    "KVMKVMKVM\0\0\0",   // KVM
    "TCGTCGTCGTCG",      // QEMU
    #if !defined(_WIN32)
    "Microsoft Hv",      // Microsoft Hyper-V or Windows Virtual PC
    #endif
    "MicrosoftXTA",      // Microsoft x86-to-ARM
    " lrpepyh  vr",      // Parallels
    "VMwareVMware",      // VMware
    "XenVMMXenVMM",      // Xen HVM
    "ACRNACRNACRN",      // Project ACRN
    " QNXQVMBSQG ",      // QNX Hypervisor
    "GenuineIntel",      // Apple Rosetta 2
    "VirtualApple"       // Newer versions of Apple Rosetta 2
  };
  for (const auto& vendor : vendors) {
    if (!memcmp(vendor, hyperVendorId, vendorIdLength)) {
      isvirtual = "YES";
      return isvirtual;
    }
  }
  isvirtual = "NO";
  return isvirtual;
  #elif (defined(__APPLE__) && defined(__MACH__))
  isvirtual = "NO";
  return isvirtual;
  #elif defined(__linux__)
  std::string tmp = read_output("echo `systemd-detect-virt 2> /dev/null`");
  std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
  if (tmp.empty()) {
    isvirtualerror = true;
    return pointer_null();
  } else if (tmp == "NONE") {
    isvirtual = "NO";
    return isvirtual;
  }
  isvirtual = "YES";
  return isvirtual;
  #endif
  // non-x86 currently not supported outside of macOS and Linux ...
  isvirtualerror = true;
  return pointer_null();
}

std::string memory_totalram(bool human_readable) {
  if (totalram != -1)
    return human_readable ? make_hreadable(totalram) : std::to_string(totalram);
  if (totalramerror)
    return pointer_null();
  #if defined(_WIN32)
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex))
    totalram = (long long)statex.ullTotalPhys;
  #elif (defined(__APPLE__) && defined(__MACH__))
  int mib[2];
  long long buf = -1;
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  std::size_t sz = sizeof(long long);
  if (!sysctl(mib, 2, &buf, &sz, nullptr, 0))
    totalram = buf;
  #elif defined(__linux__)
  totalram = read_meminfo("MemTotal");
  #elif (defined(__FreeBSD__) || defined(__DragonFly__))
  long page_s = sysconf(_SC_PAGESIZE);
  unsigned long long tram = 0;
  std::size_t sz = sizeof(tram);
  if (sysctlbyname("vm.stats.vm.v_page_count", &tram, &sz, nullptr, 0)) {
    totalramerror = true;
    return pointer_null();
  }
  if ((tram * (long long)page_s))
    totalram = (long long)(tram * (long long)page_s);
  #elif (defined(__NetBSD__) || defined(__OpenBSD__))
  int mib[2];
  mib[0] = CTL_VM;
  mib[1] = VM_UVMEXP;
  struct uvmexp buf;
  std::size_t sz = sizeof(buf);
  if (!sysctl(mib, 2, &buf, &sz, nullptr, 0))
    totalram = buf.npages * sysconf(_SC_PAGESIZE);
  #elif defined(__sun)
  totalram = (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE));
  #endif
  if (!totalram)
    totalram = -1;
  if (totalram != -1)
    return human_readable ? make_hreadable(totalram) : std::to_string(totalram);
  totalramerror = true;
  return pointer_null();
}

std::string memory_freeram(bool human_readable) {
  long long freeram = -1;
  #if defined(_WIN32)
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  if (GlobalMemoryStatusEx(&statex))
    freeram = (long long)statex.ullAvailPhys;
  #elif (defined(__APPLE__) && defined(__MACH__))
  long page_s = sysconf(_SC_PAGESIZE);
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  vm_statistics64_data_t vmstat;
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmstat, &count) == KERN_SUCCESS) {
    if ((vmstat.free_count * (long long)page_s))
      freeram = (long long)(vmstat.free_count * (long long)page_s);
  }
  #elif defined(__linux__)
  freeram = read_meminfo("MemFree");
  #elif (defined(__FreeBSD__) || defined(__DragonFly__))
  long page_s = sysconf(_SC_PAGESIZE);
  unsigned long long fram = 0;
  std::size_t sz = sizeof(fram);
  if (sysctlbyname("vm.stats.vm.v_free_count", &fram, &sz, nullptr, 0))
    return pointer_null();
  if ((fram * (long long)page_s))
    freeram = (long long)(fram * (long long)page_s);
  #elif (defined(__NetBSD__) || defined(__OpenBSD__))
  int mib[2];
  mib[0] = CTL_VM;
  mib[1] = VM_UVMEXP;
  struct uvmexp buf;
  std::size_t sz = sizeof(buf);
  if (!sysctl(mib, 2, &buf, &sz, nullptr, 0))
    freeram = buf.free * sysconf(_SC_PAGESIZE);
  #elif defined(__sun)
  freeram = (sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE));
  #endif
  if (freeram != -1)
    return human_readable ? make_hreadable(freeram) : std::to_string(freeram);
  return pointer_null();
}

std::string memory_usedram(bool human_readable) {
  long long usedram = -1;
  std::string strtotal = memory_totalram(false);
  std::string stravail = memory_freeram(false);
  long long total = ((strtotal != pointer_null()) ? strtoull(strtotal.c_str(), nullptr, 10) : -1);
  long long avail = ((stravail != pointer_null()) ? strtoull(stravail.c_str(), nullptr, 10) : -1);
  if (total != -1 && avail != -1)
    usedram = total - avail;
  if (usedram != -1)
    return human_readable ? make_hreadable(usedram) : std::to_string(usedram);
  return pointer_null();
}

std::string memory_totalswap(bool human_readable) {
  long long totalswap = -1;
  #if defined(_WIN32)
  PERFORMANCE_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  pi.cb = sizeof(pi);
  if (GetPerformanceInfo(&pi, sizeof(pi)))
    totalswap = (long long)(pi.CommitLimit * pi.PageSize);
  #elif (defined(__APPLE__) && defined(__MACH__))
  struct xsw_usage info;
  std::size_t sz = sizeof(info);
  if (!sysctlbyname("vm.swapusage", &info, &sz, nullptr, 0))
    totalswap = info.xsu_total;
  #elif defined(__linux__)
  totalswap = read_meminfo("SwapTotal");
  #elif (defined(__FreeBSD__) || defined(__DragonFly__))
  kvm_t *kvmh = nullptr;
  long page_s = sysconf(_SC_PAGESIZE);
  kvmh = kvm_open(nullptr, "/dev/null", "/dev/null", O_RDONLY, nullptr);
  if (!kvmh) {
    totalswap = -1;
    return pointer_null();
  }
  struct kvm_swap k_swap;
  if (kvm_getswapinfo(kvmh, &k_swap, 1, 0) != -1) {
    totalswap = k_swap.ksw_total * page_s;
  }
  kvm_close(kvmh);
  #elif (defined(__NetBSD__) || defined(__OpenBSD__))
  long long total = 0;
  long block_s = 0;
  int header_len = 0;
  getbsize(&header_len, &block_s);
  int nswap = swapctl(SWAP_NSWAP, nullptr, 0);
  if (!nswap) totalswap = 0;
  if (nswap > 0) {
    struct swapent *swaps = (struct swapent *)calloc(nswap, sizeof(struct swapent));
    if (swaps) {
      if (swapctl(SWAP_STATS, swaps, nswap) > 0) {
        for (int i = 0; i < nswap; i++) {
          if (swaps[i].se_flags & SWF_ENABLE)
            total += ((swaps[i].se_nblks / (1024 / block_s)) * 1024);
        }
      }
      free(swaps);
    }
    totalswap = total;
  }
  #elif defined(__sun)
  int i = 0, n = 0;
  long long total = 0;
  long page_s = sysconf(_SC_PAGESIZE);
again:
  int nswap = swapctl(SC_GETNSWP, nullptr);
  if (!nswap) totalswap = 0;
  if (nswap > 0) {
    swaptbl_t *swaps = (swaptbl_t *)malloc((nswap * sizeof(swapent_t)) + sizeof(struct swaptable));
    if (swaps) {
      char *strtab = (char *)malloc((nswap + 1) * 80);
      if (strtab) {
        for (int i = 0; i < (nswap + 1); i++)
          swaps->swt_ent[i].ste_path = strtab + (i * 80);
        swaps->swt_n = nswap + 1;
        if ((n = swapctl(SC_LIST, swaps)) > 0) {
          if (n > nswap) {
            free(strtab);
            free(swaps);
            goto again;
          }
          for (int i = 0; i < n; i++)
            total += (swaps->swt_ent[i].ste_pages * page_s);
        }
        free(strtab);
      }
      free(swaps);
    }
    totalswap = total;
  }
  #endif
  if (totalswap != -1)
    return human_readable ? make_hreadable(totalswap) : std::to_string(totalswap);
  return pointer_null();
}

std::string memory_freeswap(bool human_readable) {
  long long freeswap = -1;
  #if defined(_WIN32)
  PERFORMANCE_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  pi.cb = sizeof(pi);
  if (GetPerformanceInfo(&pi, sizeof(pi)))
    freeswap = (long long)((pi.CommitLimit - pi.CommitTotal) * pi.PageSize);
  #elif (defined(__APPLE__) && defined(__MACH__))
  struct xsw_usage info;
  std::size_t sz = sizeof(info);
  if (!sysctlbyname("vm.swapusage", &info, &sz, nullptr, 0))
    freeswap = info.xsu_avail;
  #elif defined(__linux__)
  freeswap = read_meminfo("SwapFree");
  #elif (defined(__FreeBSD__) || defined(__DragonFly__))
  kvm_t *kvmh = nullptr;
  long page_s = sysconf(_SC_PAGESIZE);
  kvmh = kvm_open(nullptr, "/dev/null", "/dev/null", O_RDONLY, nullptr);
  if (!kvmh) {
    freeswap = -1;
    return pointer_null();
  }
  struct kvm_swap k_swap;
  if (kvm_getswapinfo(kvmh, &k_swap, 1, 0) != -1) {
    freeswap = (k_swap.ksw_total - k_swap.ksw_used) * page_s;
  }
  kvm_close(kvmh);
  #elif (defined(__NetBSD__) || defined(__OpenBSD__))
  long long avail = 0;
  long block_s = 0;
  int header_len = 0;
  getbsize(&header_len, &block_s);
  int nswap = swapctl(SWAP_NSWAP, nullptr, 0);
  if (!nswap) freeswap = 0;
  if (nswap > 0) {
    struct swapent *swaps = (struct swapent *)calloc(nswap, sizeof(struct swapent));
    if (swaps) {
      if (swapctl(SWAP_STATS, swaps, nswap) > 0) {
        for (int i = 0; i < nswap; i++) {
          if (swaps[i].se_flags & SWF_ENABLE)
            avail += (((swaps[i].se_nblks - swaps[i].se_inuse) / (1024 / block_s)) * 1024);
        }
      }
      free(swaps);
    }
    freeswap = avail;
  }
  #elif defined(__sun)
  int i = 0, n = 0;
  long long avail = 0;
  long page_s = sysconf(_SC_PAGESIZE);
again:
  int nswap = swapctl(SC_GETNSWP, nullptr);
  if (!nswap) freeswap = 0;
  if (nswap > 0) {
    swaptbl_t *swaps = (swaptbl_t *)malloc((nswap * sizeof(swapent_t)) + sizeof(struct swaptable));
    if (swaps) {
      char *strtab = (char *)malloc((nswap + 1) * 80);
      if (strtab) {
        for (int i = 0; i < (nswap + 1); i++)
          swaps->swt_ent[i].ste_path = strtab + (i * 80);
        swaps->swt_n = nswap + 1;
        if ((n = swapctl(SC_LIST, swaps)) > 0) {
          if (n > nswap) {
            free(strtab);
            free(swaps);
            goto again;
          }
          for (i = 0; i < n; i++)
            avail += (swaps->swt_ent[i].ste_free * page_s);
        }
        free(strtab);
      }
      free(swaps);
    }
    freeswap = avail;
  }
  #endif
  if (freeswap != -1)
    return human_readable ? make_hreadable(freeswap) : std::to_string(freeswap);
  return pointer_null();
}

std::string memory_usedswap(bool human_readable) {
  long long usedswap = -1;
  #if defined(_WIN32)
  PERFORMANCE_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  pi.cb = sizeof(pi);
  if (GetPerformanceInfo(&pi, sizeof(pi))) {
    long long totalswap = (long long)(pi.CommitLimit * pi.PageSize);
    long long freeswap = (long long)((pi.CommitLimit - pi.CommitTotal) * pi.PageSize);
    usedswap = (long long)(totalswap - freeswap);
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  struct xsw_usage info;
  std::size_t sz = sizeof(info);
  if (!sysctlbyname("vm.swapusage", &info, &sz, nullptr, 0))
    usedswap = info.xsu_used;
  #elif defined(__linux__)
  std::string strtotal = memory_totalswap(false);
  std::string stravail = memory_freeswap(false);
  long long total = ((strtotal != pointer_null()) ? strtoull(strtotal.c_str(), nullptr, 10) : -1);
  long long avail = ((stravail != pointer_null()) ? strtoull(stravail.c_str(), nullptr, 10) : -1);
  if (total != -1 && avail != -1)
    usedswap = total - avail;
  #elif (defined(__FreeBSD__) || defined(__DragonFly__))
  kvm_t *kvmh = nullptr;
  long page_s = sysconf(_SC_PAGESIZE);
  kvmh = kvm_open(nullptr, "/dev/null", "/dev/null", O_RDONLY, nullptr);
  if (!kvmh) {
    usedswap = -1;
    return pointer_null();
  }
  struct kvm_swap k_swap;
  if (kvm_getswapinfo(kvmh, &k_swap, 1, 0) != -1) {
    usedswap = k_swap.ksw_used * page_s;
  }
  kvm_close(kvmh);
  #elif (defined(__NetBSD__) || defined(__OpenBSD__))
  long long used = 0;
  long block_s = 0;
  int header_len = 0;
  getbsize(&header_len, &block_s);
  int nswap = swapctl(SWAP_NSWAP, nullptr, 0);
  if (!nswap) usedswap = 0;
  if (nswap > 0) {
    struct swapent *swaps = (struct swapent *)calloc(nswap, sizeof(struct swapent));
    if (swaps) {
      if (swapctl(SWAP_STATS, swaps, nswap) > 0) {
        for (int i = 0; i < nswap; i++) {
          if (swaps[i].se_flags & SWF_ENABLE)
            used += ((swaps[i].se_inuse / (1024 / block_s)) * 1024);
        }
      }
      free(swaps);
    }
    usedswap = used;
  }
  #elif defined(__sun)
  int i = 0, n = 0;
  long long used = 0;
  long page_s = sysconf(_SC_PAGESIZE);
again:
  int nswap = swapctl(SC_GETNSWP, nullptr);
  if (!nswap) usedswap = 0;
  if (nswap > 0) {
    swaptbl_t *swaps = (swaptbl_t *)malloc((nswap * sizeof(swapent_t)) + sizeof(struct swaptable));
    if (swaps) {
      char *strtab = (char *)malloc((nswap + 1) * 80);
      if (strtab) {
        for (int i = 0; i < (nswap + 1); i++)
          swaps->swt_ent[i].ste_path = strtab + (i * 80);
        swaps->swt_n = nswap + 1;
        if ((n = swapctl(SC_LIST, swaps)) > 0) {
          if (n > nswap) {
            free(strtab);
            free(swaps);
            goto again;
          }
          for (i = 0; i < n; i++)
            used += ((swaps->swt_ent[i].ste_pages - swaps->swt_ent[i].ste_free) * page_s);
        }
        free(strtab);
      }
      free(swaps);
    }
    usedswap = used;
  }
  #endif
  if (usedswap != -1)
    return human_readable ? make_hreadable(usedswap) : std::to_string(usedswap);
  return pointer_null();
}

std::string gpu_manufacturer() {
  if (!gpuvendor.empty())
    return gpuvendor;
  if (gpuvendorerror)
    return pointer_null();
  #if defined(_WIN32)
  IDXGIFactory *pFactory = nullptr;
  if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory) == S_OK) {
    IDXGIAdapter *pAdapter = nullptr;
    if (pFactory->EnumAdapters(0, &pAdapter) == S_OK) {
      DXGI_ADAPTER_DESC adapterDesc;
      if (pAdapter->GetDesc(&adapterDesc) == S_OK)
        gpuvendor = get_vendor_or_device_name_by_id(adapterDesc.VendorId, false);
      pAdapter->Release();
    }
    pFactory->Release();
  }
  if (!gpuvendor.empty())
    return gpuvendor;
  #elif (!defined(__APPLE__) && !defined(__MACH__))
  #if defined(CREATE_CONTEXT)
  if (!create_context()) {
    gpuvendorerror = true;
    return pointer_null();
  }
  #endif
  #if defined(__sun)
  unsigned identifier = 0;
  std::istringstream converter(read_output("prtconf | awk '/display/{p=3} p > 0 {print $0; p--}'| awk -F'pci' 'NR==3{print $0}' | sed 's/.*pci//g' | awk -F' ' '{print $1}' | awk -F',' '{print $1}'"));
  converter >> std::hex >> identifier;
  gpuvendor = get_vendor_or_device_name_by_id(identifier, false);
  if (!gpuvendor.empty())
    return gpuvendor;
  #endif
  unsigned v = 0;
  PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC queryInteger;
  queryInteger = (PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXQueryCurrentRendererIntegerMESA");
  queryInteger(GLX_RENDERER_VENDOR_ID_MESA, &v);
  gpuvendor = v ? get_vendor_or_device_name_by_id(v, false) : "";
  if (!gpuvendor.empty())
    return gpuvendor;
  PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC queryString;
  queryString = (PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXQueryCurrentRendererStringMESA");
  const GLubyte *vendor = (GLubyte *)queryString(GLX_RENDERER_VENDOR_ID_MESA);
  gpuvendor = vendor ? (char *)vendor : "";
  if (!gpuvendor.empty())
    return gpuvendor;
  #else
  unsigned identifier = 0;
  std::istringstream converter(read_output("ioreg -bls | grep -n2 '    | |   | |   \"model\" = <\"' | awk -F',\"pci' 'NR==5{print $2}' | rev | cut -c 2- | rev | awk -F',' '{print $1}'"));
  converter >> std::hex >> identifier;
  gpuvendor = get_vendor_or_device_name_by_id(identifier, false);
  if (!gpuvendor.empty()) 
    return gpuvendor;
  gpuvendor = read_output("system_profiler SPDisplaysDataType | grep -i 'Vendor: ' | uniq | awk -F 'Vendor: ' 'NR==1{$1=$1;print}' | awk 'NR==1{$1=$1;print}'");
  std::string tmp = gpuvendor;
  std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
  if (!gpuvendor.empty() && tmp.find("APPLE") != std::string::npos)
    gpuvendor = "Apple";
  if (!gpuvendor.empty()) 
    return gpuvendor;
  #endif
  gpuvendorerror = true;
  return pointer_null();
}

std::string gpu_renderer() {
  if (!gpurenderer.empty())
    return gpurenderer;
  if (gpurenderererror)
    return pointer_null();
  std::string result;
  #if defined(_WIN32)
  auto narrow = [](std::wstring wstr) {
    if (wstr.empty()) return std::string("");
    int nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);
    std::vector<char> buf(nbytes);
    return std::string { buf.data(), (std::size_t)WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), buf.data(), nbytes, nullptr, nullptr) };
  };
  IDXGIFactory *pFactory = nullptr;
  if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory) == S_OK) {
    IDXGIAdapter *pAdapter = nullptr;
    if (pFactory->EnumAdapters(0, &pAdapter) == S_OK) {
      DXGI_ADAPTER_DESC adapterDesc;
      if (pAdapter->GetDesc(&adapterDesc) == S_OK) {
        gpurenderer = get_vendor_or_device_name_by_id(adapterDesc.DeviceId, true);
        if (!gpurenderer.empty()) 
          return gpurenderer;
        gpurenderer = narrow(adapterDesc.Description);
        if (!gpurenderer.empty()) 
          return gpurenderer;
      }
      pAdapter->Release();
    }
    pFactory->Release();
  }
  #elif (!defined(__APPLE__) && !defined(__MACH__))
  #if defined(CREATE_CONTEXT)
  if (!create_context()) {
    gpurenderererror = true;
    return pointer_null();
  }
  #endif
  #if defined(__sun)
  unsigned identifier = 0;
  std::istringstream converter(read_output("prtconf | awk '/display/{p=3} p > 0 {print $0; p--}' | awk -F'pci' 'NR==3{print $0}' | sed 's/.*pci//g' | awk -F' ' '{print $1}' | awk -F',' '{print $2}'"));
  converter >> std::hex >> identifier;
  gpurenderer = get_vendor_or_device_name_by_id(identifier, true);
  if (!gpurenderer.empty())
    return gpurenderer;
  #endif
  unsigned v = 0;
  PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC queryInteger;
  queryInteger = (PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXQueryCurrentRendererIntegerMESA");
  queryInteger(GLX_RENDERER_DEVICE_ID_MESA, &v);
  gpurenderer = v ? get_vendor_or_device_name_by_id(v, true) : "";
  if (!gpurenderer.empty())
    return gpurenderer;
  PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC queryString;
  queryString = (PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXQueryCurrentRendererStringMESA");
  const GLubyte *renderer = (GLubyte *)queryString(GLX_RENDERER_DEVICE_ID_MESA);
  gpurenderer = renderer ? (char *)renderer : "";
  if (!gpurenderer.empty())
    return gpurenderer;
  #else
  unsigned identifier = 0;
  std::istringstream converter(read_output("ioreg -bls | grep -n2 '    | |   | |   \"model\" = <\"' | awk -F',\"pci' 'NR==5{print $2}' | rev | cut -c 2- | rev | awk -F',' '{print $2}'"));
  converter >> std::hex >> identifier;
  gpurenderer = get_vendor_or_device_name_by_id(identifier, true);
  if (!gpurenderer.empty()) 
    return gpurenderer;
  gpurenderer = read_output("system_profiler SPDisplaysDataType | grep -i 'Chipset Model: ' | uniq | awk -F 'Chipset Model: ' 'NR==1{$1=$1;print}' | awk 'NR==1{$1=$1;print}'");
  if (!gpurenderer.empty()) 
    return gpurenderer;
  #endif
  gpurenderererror = true;
  return pointer_null();
}

std::string memory_totalvram(bool human_readable) {
  if (videomemory != -1) 
    return human_readable ? make_hreadable(videomemory) : std::to_string(videomemory);
  if (videomemoryerror) 
    return pointer_null();
  #if defined(_WIN32)
  IDXGIFactory *pFactory = nullptr;
  if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory) == S_OK) {
    IDXGIAdapter *pAdapter = nullptr;
    if (pFactory->EnumAdapters(0, &pAdapter) == S_OK) {
      DXGI_ADAPTER_DESC adapterDesc;
      if (pAdapter->GetDesc(&adapterDesc) == S_OK)
        videomemory = (long long)adapterDesc.DedicatedVideoMemory;
      pAdapter->Release();
    }
    pFactory->Release();
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  videomemory = strtoll(read_output("ioreg -r -d 1 -w 0 -c \"IOAccelerator\" | grep '\"VRAM,totalMB\"' | uniq | awk -F '= ' '{print $2}'").c_str(), nullptr, 10) * 1024 * 1024;
  if (!videomemory) {
    videomemory = shmem::getsharedmemory();
  }
  #else
  #if defined(CREATE_CONTEXT)
  if (!create_context()) {
    videomemory = -1;
    videomemoryerror = true;
    return pointer_null();
  }
  #endif
  unsigned v = 0;
  PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC queryInteger;
  queryInteger = (PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXQueryCurrentRendererIntegerMESA");
  queryInteger(GLX_RENDERER_VIDEO_MEMORY_MESA, &v);
  videomemory = v * 1024 * 1024;
  #endif
  if (!videomemory)
    videomemory = -1;
  if (videomemory != -1) 
    return human_readable ? make_hreadable(videomemory) : std::to_string(videomemory);
  videomemoryerror = true;
  return pointer_null();
}

std::string cpu_vendor() {
  if (!cpuvendor.empty())
    return cpuvendor;
  if (cpuvendorerror)
    return pointer_null();
  #if defined(_WIN32)
  char buf[1024];
  DWORD sz = sizeof(buf);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\", "VendorIdentifier", RRF_RT_REG_SZ, nullptr, &buf, &sz) == ERROR_SUCCESS)
    cpuvendor = buf;
  #elif (defined(__APPLE__) && defined(__MACH__))
  char buf[1024];
  std::size_t sz = sizeof(buf);
  if (!sysctlbyname("machdep.cpu.vendor", &buf, &sz, nullptr, 0))
    cpuvendor = buf;
  #elif defined(__linux__)
  cpuvendor = read_output("lscpu | grep 'Vendor ID:' | uniq | cut -d' ' -f3- | awk 'NR==1{$1=$1;print}'");
  #elif (defined(__FreeBSD__) || defined(__DragonFly__))
  #if (defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86))
  /* free / dragonfly bsd have no api for getting the cpu vendor; 
  use x86-specific inline assembly. If the current platform isn't 
  x86 or x64 this fails and the fallback will be check for ARM */
  class cpuid {
    unsigned regs[4];
  public:
    explicit cpuid(unsigned func_id, unsigned sub_func_id) {
      asm volatile ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (func_id), "c" (sub_func_id));
    }
    #if defined(_MSC_VER)
    const int &eax() const {
      return regs[0];
    }
    const int &ebx() const {
      return regs[1];
    }
    const int &ecx() const {
      return regs[2];
    }
    const int &edx() const {
      return regs[3];
    }
    #else
    const unsigned &eax() const {
      return regs[0];
    }
    const unsigned &ebx() const {
      return regs[1];
    }
    const unsigned &ecx() const {
      return regs[2];
    }
    const unsigned &edx() const {
      return regs[3];
    }
    #endif
  };
  cpuid cpuid0(0, 0);
  cpuvendor += std::string((const char *)&cpuid0.ebx(), 4);
  cpuvendor += std::string((const char *)&cpuid0.edx(), 4);
  cpuvendor += std::string((const char *)&cpuid0.ecx(), 4);
  #endif
  #elif defined(__NetBSD__)
  cpuvendor = read_output("cat /proc/cpuinfo | grep 'vendor_id' | awk 'NR==1{print $3}'");
  #elif defined(__OpenBSD__)
  cpuvendor = read_output("sysctl -n machdep.cpuvendor");
  #elif defined(__sun)
  cpuvendor = read_output("psrinfo -v -p | awk 'NR==2{print substr($2, 2)}'");
  #endif
  std::string tmp1 = cpu_processor();
  std::transform(tmp1.begin(), tmp1.end(), tmp1.begin(), ::toupper);
  if (cpuvendor.empty() && tmp1.find("APPLE") != std::string::npos)
    cpuvendor = "Apple";
  std::string tmp2 = os_architecture();
  std::transform(tmp2.begin(), tmp2.end(), tmp2.begin(), ::toupper);
  if (cpuvendor.empty() && (tmp2.find("ARM") != std::string::npos || 
    tmp2.find("AARCH64") != std::string::npos))
    cpuvendor = "ARM";
  while (!cpuvendor.empty() && cpuvendor.back() == ' ')
    cpuvendor.pop_back();
  if (!cpuvendor.empty())
    return cpuvendor;
  cpuvendorerror = true;
  return pointer_null();
}

std::string cpu_processor() {
  if (!cpubrand.empty())
    return cpubrand;
  if (cpubranderror)
    return pointer_null();
  #if defined(_WIN32)
  char buf[1024];
  DWORD sz = sizeof(buf);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\", "ProcessorNameString", RRF_RT_REG_SZ, nullptr, &buf, &sz) == ERROR_SUCCESS)
    cpubrand = buf;
  #elif (defined(__APPLE__) && defined(__MACH__))
  char buf[1024];
  std::size_t sz = sizeof(buf);
  if (!sysctlbyname("machdep.cpu.brand_string", &buf, &sz, nullptr, 0))
    cpubrand = buf;
  #elif defined(__linux__)
  cpubrand = read_output("lscpu | grep 'Model name:' | uniq | cut -d' ' -f3- | awk 'NR==1{$1=$1;print}'");
  #elif (defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || defined(__OpenBSD__))
  int mib[2];
  char buf[1024];
  mib[0] = CTL_HW;
  mib[1] = HW_MODEL;
  std::size_t sz = sizeof(buf);
  if (!sysctl(mib, 2, buf, &sz, nullptr, 0))
    cpubrand = buf;
  #elif defined(__sun)
  cpubrand = read_output("psrinfo -v -p | awk 'NR==3{print}' | awk 'NR==1{$1=$1;print}''");
  #endif
  while (!cpubrand.empty() && cpubrand.back() == ' ')
    cpubrand.pop_back();
  if (!cpubrand.empty())
    return cpubrand;
  cpubranderror = true;
  return pointer_null();
}

std::string cpu_core_count() {
  if (numcores != -1)
    return std::to_string(numcores);
  if (numcoreserror)
    return pointer_null();
  #if defined(_WIN32)
  std::string tmp = read_output("C:\\Windows\\System32\\wbem\\WMIC.exe cpu get NumberOfCores");
  if (!tmp.empty()) {
    tmp = std::regex_replace(tmp, std::regex("NumberOfCores"), "");
    tmp = std::regex_replace(tmp, std::regex(" "), "");
    tmp = std::regex_replace(tmp, std::regex("\t"), "");
    tmp = std::regex_replace(tmp, std::regex("\r"), "");
    tmp = std::regex_replace(tmp, std::regex("\n"), "");
    numcores = (int)strtol(tmp.c_str(), nullptr, 10);
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  int buf = -1;
  std::size_t sz = sizeof(int);
  if (!sysctlbyname("machdep.cpu.core_count", &buf, &sz, nullptr, 0))
    numcores = buf;
  #elif defined(__linux__)
  numcores = (int)(strtol(read_output("echo `lscpu | awk '/^Socket\\(s\\)/{ print $2 }'`").c_str(), nullptr, 10) * 
    strtol(read_output("echo `lscpu | awk '/^Core\\(s\\) per socket/{ print $4 }'`").c_str(), nullptr, 10));
  #elif defined(__FreeBSD__)
  int buf = -1;
  std::size_t sz = sizeof(int);
  if (!sysctlbyname("kern.smp.cores", &buf, &sz, nullptr, 0))
    numcores = buf;
  #elif (defined(__DragonFly__) || defined(__NetBSD__) || defined(__sun))
  hwloc_topology_t topology = nullptr;
  if (!hwloc_topology_init(&topology)) {
    if (!hwloc_topology_load(topology)) {
      int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
      if (depth != HWLOC_TYPE_DEPTH_UNKNOWN)
        numcores = hwloc_get_nbobjs_by_depth(topology, depth);
    }
    hwloc_topology_destroy(topology);
  }
  #endif
  if (!numcores)
    numcores = -1;
  if (numcores != -1)
    return std::to_string(numcores);
  numcoreserror = true;
  return pointer_null();
}

std::string cpu_processor_count() {
  if (numcpus != -1)
    return std::to_string(numcpus);
  if (numcpuserror)
    return pointer_null();
  #if defined(_WIN32)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  numcpus = sysinfo.dwNumberOfProcessors;
  #elif (defined(__APPLE__) && defined(__MACH__))
  int buf = -1;
  std::size_t sz = sizeof(int);
  if (!sysctlbyname("machdep.cpu.thread_count", &buf, &sz, nullptr, 0))
    numcpus = buf;
  #elif defined(__linux__)
  numcpus = (int)strtol(read_output("lscpu | grep 'CPU(s):' | uniq | cut -d' ' -f4- | awk 'NR==1{$1=$1;print}'").c_str(), nullptr, 10);
  #elif (defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || defined(__OpenBSD__))
  int mib[2];
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  int buf = -1;
  std::size_t sz = sizeof(int);
  if (!sysctl(mib, 2, &buf, &sz, nullptr, 0))
    numcpus = buf;
  #elif defined(__sun)
  numcpus = (int)strtol(read_output("psrinfo | wc -l | awk '{print $1}'").c_str(), nullptr, 10);
  #endif
  if (!numcpus)
    numcpus = -1;
  if (numcpus != -1)
    return std::to_string(numcpus);
  numcpuserror = true;
  return pointer_null();
}

} // namespace ngs::sys
