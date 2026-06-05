/*

MIT License

Copyright © 2016 Andre Leiradella
Copyright © 2021-2025 Samuel Venable

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

#include "imguial_msgbox.h"

#include <apifilesystem/filesystem.hpp>
#include <apifilesystem/ghc/filesystem.hpp>
#include <apifiledialogs/filedialogs.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#if !defined(_WIN32)
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool init = false;
extern SDL_Window *dialog;
ImGuiAl::MsgBox::~MsgBox() { }

void AlignForWidth(float width, float alignment = 0.5f) {
  ImGuiStyle& style = ImGui::GetStyle();
  float avail = ImGui::GetContentRegionAvail().x;
  float off = (avail - width) * alignment;
  if (off > 0.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
  }
  ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - (ImGui::GetFontSize() + (ImGui::GetFontSize() / 2)));
}

std::string ReplaceAllUtf8CharsWithAsterisk(std::string str) {
  if (str.empty()) return ""; std::string res; 
  unsigned char *p = (unsigned char *)str.c_str();
  while (*p != '\0') {
    if ((*p & 0xC0) != 0x80) {
      res += "*";
    }
    p++;
  }
  return res;
}

std::string string_receive() {
  std::string str;
  #if defined(_WIN32)
  std::string pipeName = std::string("\\\\.\\pipe\\IMGUI_DIALOG_PIPE_") + 
  ngs::fs::environment_get_variable("IMGUI_DIALOG_PID");
  HANDLE hPipe = CreateFileA(pipeName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (hPipe == INVALID_HANDLE_VALUE) {
    return "";
  }
  DWORD nRead = 0; 
  char buffer[4096];
  while (ReadFile(hPipe, buffer, 4096, &nRead, nullptr) && nRead) {
    buffer[nRead] = '\0';
    str.append(buffer, nRead);
  }
  CloseHandle(hPipe);
  #else
  int fd = 0;
  struct stat st;
  std::string pipeName = std::string("/tmp/IMGUI_DIALOG_PIPE_") + 
  ngs::fs::environment_get_variable("IMGUI_DIALOG_PID");
  if (stat(pipeName.c_str(), &st) == 0 && S_ISFIFO(st.st_mode)) {
    if (mkfifo(pipeName.c_str(), 0666) != 0) {
      if (errno != EEXIST) {
        return "";
      }
    }
    fd = open(pipeName.c_str(), O_RDONLY);
    if (fd == -1) {
      return "";
    }
    ssize_t nRead = 0;
    char buffer[4096];
    while ((nRead = read(fd, buffer, 4096)) > 0) {
      buffer[nRead] = '\0';
      str.append(buffer, nRead);
    }
    close(fd);
  }
  #endif
  return str;
}

bool ImGuiAl::MsgBox::Init(const char *title, const char *text, std::vector<std::string> captions, bool input) {
  m_Title = title;
  m_Text = text;
  m_Captions = captions;
  m_Input = input;
  return true;
}

static std::string strrecv;
int ImGuiAl::MsgBox::Draw() {
  int index = 0;
  if (ImGui::BeginPopupModal(m_Title, nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | 
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
    std::string str = string_receive();
    if (!str.empty()) strrecv = str;
    str = strrecv.empty() ? m_Text : strrecv;
    ImGui::TextWrapped(str.c_str());
    int sw = 0, sh = 0;
    int dw = ImGui::CalcTextSize(m_Text, m_Text + strlen(m_Text), false, 100 * (0.25 * ImGui::GetFontSize())).x;
    if (dw < ImGui::GetWindowContentRegionMax().x * 0.75) dw = ImGui::GetWindowContentRegionMax().x * 0.75;
    int dh = ImGui::CalcTextSize(m_Text, m_Text + strlen(m_Text), false, 100 * (0.25 * ImGui::GetFontSize())).y + (4.875f * ImGui::GetFontSize());
    if (m_Input) dh += ((4.875f * ImGui::GetFontSize()) / 2);
    if (dialog && ngs::fs::environment_get_variable("IMGUI_DIALOG_FULLSCREEN") != std::to_string(1)) {
      SDL_GetWindowSize(dialog, &sw, &sh);
      SDL_SetWindowSize(dialog, dw, dh);
      #if defined(_WIN32)
      if ((!ngs::fs::environment_get_variable("IMGUI_DIALOG_PARENT").empty() &&
      ngs::fs::environment_get_variable("IMGUI_DIALOG_EMBEDDED") != std::to_string(1)) ||
      ngs::fs::environment_get_variable("IMGUI_DIALOG_PARENT").empty())
      #endif
      SDL_SetWindowPosition(dialog, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    ImGui::Separator();
    ImVec2 size = ImVec2(4.875f * ImGui::GetFontSize(), 0.0f);
    int count;
    ImGuiStyle& style = ImGui::GetStyle();
    float width = 0.0f;
    for (count = 0; count < m_Captions.size(); count++) {
      width += size.x;
      width += style.ItemSpacing.x;
    }
    width -= style.ItemSpacing.x;
    bool enter_pressed = false;
    float avail = ImGui::GetContentRegionAvail().x;
    if (m_Input) {
      ImGui::SetNextItemWidth(avail);
      ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - (4 * ImGui::GetFontSize()));
      if (!init && !ImGui::IsMouseClicked(0)) {
        strcpy(m_Value, Value);
        init = true;
      }
      enter_pressed = ImGui::InputTextEx("##inputBox", 
      ((ngs::fs::environment_get_variable("IMGUI_DIALOG_PASSWORD") == std::to_string(1)) ? ReplaceAllUtf8CharsWithAsterisk(m_Value).c_str() : m_Value),
      Value, 1024, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue | 
      ((ngs::fs::environment_get_variable("IMGUI_DIALOG_PASSWORD") == std::to_string(1)) ? ImGuiInputTextFlags_Password : 0));
    }
    AlignForWidth(width);
    for (count = 0; count < m_Captions.size(); count++) {
      ImGui::PushID(count);
      enter_pressed = (ImGui::Button(m_Captions[count].c_str(), size) || ImGui::IsItemClicked(0) || enter_pressed);
      if (enter_pressed) {
        index = count + 1;
        ImGui::CloseCurrentPopup();
        ImGui::PopID();
        break;
      }
      ImGui::SameLine();
      ImGui::PopID();
    }
    size = ImVec2((4 - count) * 4.875f * ImGui::GetFontSize(), ImGui::GetFontSize());
    ImGui::Dummy(size);
    ImGui::EndPopup();
  }
  Result = Value;
  init = false;
  return index;
}

void ImGuiAl::MsgBox::Open() {
  ImGui::OpenPopup(m_Title);
}
