#include <string>

namespace enigma_user {

  static string ws_win32       = "Win32";
  static string ws_cocoa       = "Cocoa";
  static string ws_x11_zenity  = "Zenity";
  static string ws_x11_kdialog = "KDialog";

  static const int btn_abort  = 0;
  static const int btn_ignore = 1;
  static const int btn_ok     = 2;
  static const int btn_cancel = 3;
  static const int btn_yes    = 4;
  static const int btn_no     = 5;
  static const int btn_retry  = 6;

  int show_message_cancelable(std::string str);
  int show_question(std::string str);
  int show_question_cancelable(std::string str);
  int show_attempt(std::string str);
  std::string get_password(std::string str, std::string def);
  double get_passcode(std::string str, double def);
  std::string get_open_filename(std::string filter, std::string fname);
  std::string get_open_filename_ext(std::string filter, std::string fname, std::string dir, std::string title);
  std::string get_open_filenames(std::string filter, std::string fname);
  std::string get_open_filenames_ext(std::string filter, std::string fname, std::string dir, std::string title);
  std::string get_save_filename(std::string filter, std::string fname);
  std::string get_save_filename_ext(std::string filter, std::string fname, std::string dir, std::string title);
  std::string get_directory(std::string dname);
  std::string get_directory_alt(std::string capt, std::string root);
  int get_color(int defcol);
  int get_color_ext(int defcol, std::string title);
  std::string widget_get_caption();
  void widget_set_caption(std::string str);
  std::string widget_get_owner();
  void widget_set_owner(std::string hwnd);
  std::string widget_get_icon();
  void widget_set_icon(std::string icon);
  std::string widget_get_system();
  void widget_set_system(std::string sys);
  std::string widget_get_button_name(int type);
  void widget_set_button_name(int type, std::string name);
  bool widget_get_canceled();
  inline bool action_if_question(std::string str) {
    return show_question(str);
  }

} // namespace enigma_user
