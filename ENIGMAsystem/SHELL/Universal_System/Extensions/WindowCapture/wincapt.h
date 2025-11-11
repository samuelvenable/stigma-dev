/*

 MIT License

 Copyright © 2025 Samuel Venable

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

#include "Platforms/platforms_mandatory.h"
#include "Universal_System/var4.h"

namespace enigma_user {

  int capture_add(enigma::rvt window);
  bool capture_exists(int ind);
  bool capture_delete(int ind);
  int capture_get_width(int ind);
  int capture_get_height(int ind);
  bool capture_grab_frame_buffer(int ind, void *buffer);
  bool capture_update(int ind);
  int capture_create_window_list();
  bool capture_window_list_exists(int list);
  wid_t capture_get_window_id(int list, int ind);
  int capture_get_window_id_length(int list);
  bool capture_destroy_window_list(int list);
  std::string capture_get_window_caption(wid_t window);
  bool capture_get_fixedsize(int ind);
  bool capture_set_fixedsize(int ind, bool fixed);
  window_t capture_native_window_from_window_id(wid_t window);
  wid_t capture_window_id_from_native_window(enigma::rvt window);

} // namespace enigma_user
