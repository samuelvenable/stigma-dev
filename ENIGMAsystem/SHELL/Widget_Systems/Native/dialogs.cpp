#include "Widget_Systems/Native/libdlgmod/libdlgmod/libdlgmod.h"
#include "Widget_Systems/widgets_mandatory.h"
#include "Widget_Systems/Native/dialogs.h"
#include "Platforms/General/PFwindow.h"
#include "Platforms/General/PFmain.h"

namespace enigma {

  bool widgets_initialized = false;
  bool widget_system_initialize() {
    if (!widgets_initialized) {
      ::widget_set_owner(std::to_string((unsigned long long)(void *)enigma_user::window_handle()).c_str());
      widgets_initialized = true;
    }
    return widgets_initialized;
  }

} // namespace enigma

namespace enigma_user {

  void show_info() { }

  int show_debug_message(std::string str, MESSAGE_TYPE type) {
    enigma::widget_system_initialize();
    int result = -1;
    #ifndef DEBUG_MODE
    str += "\n";
    fputs(str.c_str(), stderr);
    fflush(stderr);
    #endif
    if (type == MESSAGE_TYPE::M_FATAL_ERROR || 
      type == MESSAGE_TYPE::M_FATAL_USER_ERROR) {
      std::string caption = ::widget_get_caption();
      ::widget_set_caption("Fatal Error");
      result = ::show_error(str.c_str(), true);
      ::widget_set_caption(caption.c_str());
      abort();
    } else if (type == MESSAGE_TYPE::M_ERROR || 
      type == MESSAGE_TYPE::M_USER_ERROR) {
      std::string caption = ::widget_get_caption();
      ::widget_set_caption("Error");
      result = ::show_error(str.c_str(), false);
      ::widget_set_caption(caption.c_str());
    }
    return result;
  }

  int show_message(std::string str) {
    enigma::widget_system_initialize();
    return ::show_message(str.c_str());
  }

  int show_message_cancelable(std::string str) {
    enigma::widget_system_initialize();
    return ::show_message_cancelable(str.c_str());
  }

  int show_question(std::string str) {
    enigma::widget_system_initialize();
    return ::show_question(str.c_str());
  }

  int show_question_cancelable(std::string str) {
    enigma::widget_system_initialize();
    return ::show_question_cancelable(str.c_str());
  }

  int show_attempt(std::string str) {
    enigma::widget_system_initialize();
    return ::show_attempt(str.c_str());
  }

  std::string get_string(std::string str, std::string def) {
    enigma::widget_system_initialize();
    return ::get_string(str.c_str(), def.c_str());
  }

  std::string get_password(std::string str, std::string def) {
    enigma::widget_system_initialize();
    return ::get_password(str.c_str(), def.c_str());
  }

  double get_integer(std::string str, double def) {
    enigma::widget_system_initialize();
    return ::get_integer(str.c_str(), def);
  }

  double get_passcode(std::string str, double def) {
    enigma::widget_system_initialize();
    return ::get_passcode(str.c_str(), def);
  }

  std::string get_open_filename(std::string filter, std::string fname) {
    enigma::widget_system_initialize();
    return ::get_open_filename(filter.c_str(), fname.c_str());
  }

  std::string get_open_filename_ext(std::string filter, std::string fname, std::string dir, std::string title) {
    enigma::widget_system_initialize();
    return ::get_open_filename_ext(filter.c_str(), fname.c_str(), dir.c_str(), title.c_str());
  }

  std::string get_open_filenames(std::string filter, std::string fname) {
    enigma::widget_system_initialize();
    return ::get_open_filenames(filter.c_str(), fname.c_str());
  }

  std::string get_open_filenames_ext(std::string filter, std::string fname, std::string dir, std::string title) {
    enigma::widget_system_initialize();
    return ::get_open_filenames_ext(filter.c_str(), fname.c_str(), dir.c_str(), title.c_str());
  }

  std::string get_save_filename(std::string filter, std::string fname) {
    enigma::widget_system_initialize();
    return ::get_save_filename(filter.c_str(), fname.c_str());
  }

  std::string get_save_filename_ext(std::string filter, std::string fname, std::string dir, std::string title) {
    enigma::widget_system_initialize();
    return ::get_save_filename_ext(filter.c_str(), fname.c_str(), dir.c_str(), title.c_str());
  }

  std::string get_directory(std::string dname) {
    enigma::widget_system_initialize();
    return ::get_directory(dname.c_str());
  }

  std::string get_directory_alt(std::string capt, std::string root) {
    enigma::widget_system_initialize();
    return ::get_directory_alt(capt.c_str(), root.c_str());
  }

  int get_color(int defcol) {
    enigma::widget_system_initialize();
    return ::get_color(defcol);
  }

  int get_color_ext(int defcol, std::string title) {
    enigma::widget_system_initialize();
    return ::get_color_ext(defcol, title.c_str());
  }

  std::string widget_get_caption() {
    enigma::widget_system_initialize();
    return ::widget_get_caption();
  }

  void widget_set_caption(std::string str) {
    enigma::widget_system_initialize();
    ::widget_set_caption(str.c_str());
  }

  std::string widget_get_owner() {
    enigma::widget_system_initialize();
    return ::widget_get_owner();
  }

  void widget_set_owner(std::string hwnd) {
    enigma::widget_system_initialize();
    ::widget_set_owner(hwnd.c_str());
  }

  std::string widget_get_icon() {
    enigma::widget_system_initialize();
    return ::widget_get_icon();
  }

  void widget_set_icon(std::string icon) {
    enigma::widget_system_initialize();
    ::widget_set_icon(icon.c_str());
  }

  std::string widget_get_system() {
    enigma::widget_system_initialize();
    return ::widget_get_system();
  }

  void widget_set_system(std::string sys) {
    enigma::widget_system_initialize();
    ::widget_set_system(sys.c_str());
  }

  std::string widget_get_button_name(int type) {
    enigma::widget_system_initialize();
    return ::widget_get_button_name(type);
  }

  void widget_set_button_name(int type, std::string name) {
    enigma::widget_system_initialize();
    ::widget_set_button_name(type, name.c_str());
  }

  bool widget_get_canceled() {
    enigma::widget_system_initialize();
    return ::widget_get_canceled();
  }

} // namespace enigma_user
