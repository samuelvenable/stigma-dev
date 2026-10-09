/*

MIT License

Copyright © 2021-2026 Samuel Venable

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

#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <clocale>

#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#include <libdlgmod/libdlgmod.h>

#include <windows.h>
#include <gdiplus.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <comdef.h>
#ifdef _MSC_VER
#include <atlbase.h>
#include <activscp.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <share.h>
#include <io.h>
#endif
#include <objbase.h>
#include <shlobj.h>

#if !defined(_MSC_VER)
#include <xprocess.hpp>
#endif

using namespace Gdiplus;
using std::basic_string;
using std::stringstream;
using std::to_string;
using std::wstring;
using std::string;
using std::vector;
using std::size_t;

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#endif

namespace dialog_module {

  namespace {

    // window handles
    void *owner = nullptr;
    HWND parent = nullptr;
    HWND dlg = nullptr;
    HWND win = nullptr;

    // hook procs
    vector<HWND> hwnds;
    HHOOK hhook = nullptr;

    // error msgs
    bool fatal = false;

    // input boxes
    bool hidden = false;

    // get color
    string tstr_gctitle;
    wstring cpp_wstr_gctitle;

    // file dialogs
    OPENFILENAMEW ofn; 
    bool cancel_pressed = false;
    wchar_t *wstr_filter = nullptr;
    wchar_t wstr_fname[32767];
    wstring cpp_wstr_dir;
    wstring cpp_wstr_title;
    wstring pipefilter;
    HWND textbox;
    string files;

    // misc
    string caption;
    string tstr_icon;

    enum BUTTON_TYPES {
      BUTTON_ABORT,
      BUTTON_IGNORE,
      BUTTON_OK,
      BUTTON_CANCEL,
      BUTTON_YES,
      BUTTON_NO,
      BUTTON_RETRY
    };

    enum CAPTION_TYPES {
      CAPTION_INFORMATION,
      CAPTION_QUESTION,
      CAPTION_INPUT,
      CAPTION_OPEN,
      CAPTION_SAVE,
      CAPTION_DIRECTORY,
      CAPTION_COLOR,
      CAPTION_ERROR
    };

    int const btn_array_len = 7;
    string btn_array[btn_array_len] = { "Abort", "Ignore", "OK", "Cancel", "Yes", "No", "Retry" };

    int const cpt_array_len = 8;
    string cpt_array[cpt_array_len] = { "Information", "Question", "Input Query", "Open", "Save As", "Select Directory", "Color", "Error" };

    void widget_set_locale_helper() {
      const char *ptr = std::setlocale(LC_ALL, "");  
      if (ptr && strlen(ptr) >= 2) {
        if (ptr[0] == 'e' && ptr[1] == 'n') {
          // en = English

          btn_array[BUTTON_ABORT] = "Abort";
          btn_array[BUTTON_IGNORE] = "Ignore";
          btn_array[BUTTON_OK] = "OK";
          btn_array[BUTTON_CANCEL] = "Cancel";
          btn_array[BUTTON_YES] = "Yes";
          btn_array[BUTTON_NO] = "No";
          btn_array[BUTTON_RETRY] = "Retry";

          cpt_array[CAPTION_INFORMATION] = "Information";
          cpt_array[CAPTION_QUESTION] = "Question";
          cpt_array[CAPTION_INPUT] = "Input Query";
          cpt_array[CAPTION_OPEN] = "Open";
          cpt_array[CAPTION_SAVE] = "Save As";
          cpt_array[CAPTION_DIRECTORY] = "Select Directory";
          cpt_array[CAPTION_COLOR] = "Color";
          cpt_array[CAPTION_ERROR] = "Error";

        } else if (ptr[0] == 'f' && ptr[1] == 'r') {
          // fr = French

          btn_array[BUTTON_ABORT] = "Avorter";
          btn_array[BUTTON_IGNORE] = "Ignorer";
          btn_array[BUTTON_OK] = "D'ACCORD";
          btn_array[BUTTON_CANCEL] = "Annuler";
          btn_array[BUTTON_YES] = "Oui";
          btn_array[BUTTON_NO] = "Non";
          btn_array[BUTTON_RETRY] = "Réessayer";

          cpt_array[CAPTION_INFORMATION] = "Information";
          cpt_array[CAPTION_QUESTION] = "Question";
          cpt_array[CAPTION_INPUT] = "Requête d'entrée";
          cpt_array[CAPTION_OPEN] = "Ouvrir";
          cpt_array[CAPTION_SAVE] = "Enregistrer sous";
          cpt_array[CAPTION_DIRECTORY] = "Sélectionner un répertoire";
          cpt_array[CAPTION_COLOR] = "Couleur";
          cpt_array[CAPTION_ERROR] = "Erreur";

        } else if (ptr[0] == 'd' && ptr[1] == 'e') {
          // de = German

          btn_array[BUTTON_ABORT] = "Abbrechen";
          btn_array[BUTTON_IGNORE] = "Ignorieren";
          btn_array[BUTTON_OK] = "OK";
          btn_array[BUTTON_CANCEL] = "Stornieren";
          btn_array[BUTTON_YES] = "Ja";
          btn_array[BUTTON_NO] = "NEIN";
          btn_array[BUTTON_RETRY] = "Wiederholen";

          cpt_array[CAPTION_INFORMATION] = "Information";
          cpt_array[CAPTION_QUESTION] = "Frage";
          cpt_array[CAPTION_INPUT] = "Eingabeabfrage";
          cpt_array[CAPTION_OPEN] = "Offen";
          cpt_array[CAPTION_SAVE] = "Speichern unter";
          cpt_array[CAPTION_DIRECTORY] = "Verzeichnis auswählen";
          cpt_array[CAPTION_COLOR] = "Farbe";
          cpt_array[CAPTION_ERROR] = "Fehler";

        } else if (ptr[0] == 'e' && ptr[1] == 's') {
          // es = Spanish

          btn_array[BUTTON_ABORT] = "Abortar";
          btn_array[BUTTON_IGNORE] = "Ignorar";
          btn_array[BUTTON_OK] = "DE ACUERDO";
          btn_array[BUTTON_CANCEL] = "Cancelar";
          btn_array[BUTTON_YES] = "Sí";
          btn_array[BUTTON_NO] = "No";
          btn_array[BUTTON_RETRY] = "Rever";

          cpt_array[CAPTION_INFORMATION] = "Información";
          cpt_array[CAPTION_QUESTION] = "Pregunta";
          cpt_array[CAPTION_INPUT] = "Consulta de entrada";
          cpt_array[CAPTION_OPEN] = "Abierta";
          cpt_array[CAPTION_SAVE] = "Guardar como";
          cpt_array[CAPTION_DIRECTORY] = "Seleccionar directorio";
          cpt_array[CAPTION_COLOR] = "Color";
          cpt_array[CAPTION_ERROR] = "Error";

        } else if (ptr[0] == 'z' && ptr[1] == 'h') {
          // zh = Chinese
  
          btn_array[BUTTON_ABORT] = "中止";
          btn_array[BUTTON_IGNORE] = "忽略";
          btn_array[BUTTON_OK] = "好的";
          btn_array[BUTTON_CANCEL] = "取消";
          btn_array[BUTTON_YES] = "是的";
          btn_array[BUTTON_NO] = "不";
          btn_array[BUTTON_RETRY] = "重试";

          cpt_array[CAPTION_INFORMATION] = "信息";
          cpt_array[CAPTION_QUESTION] = "问题";
          cpt_array[CAPTION_INPUT] = "输入查询";
          cpt_array[CAPTION_OPEN] = "打开";
          cpt_array[CAPTION_SAVE] = "另存为";
          cpt_array[CAPTION_DIRECTORY] = "选择目录";
          cpt_array[CAPTION_COLOR] = "颜色";
          cpt_array[CAPTION_ERROR] = "错误";

        } else if (ptr[0] == 'j' && ptr[1] == 'a') {
          // ja = Japanese
  
          btn_array[BUTTON_ABORT] = "アボート";
          btn_array[BUTTON_IGNORE] = "無視する";
          btn_array[BUTTON_OK] = "わかりました";
          btn_array[BUTTON_CANCEL] = "キャンセル";
          btn_array[BUTTON_YES] = "はい";
          btn_array[BUTTON_NO] = "いいえ";
          btn_array[BUTTON_RETRY] = "リトライ";

          cpt_array[CAPTION_INFORMATION] = "情報";
          cpt_array[CAPTION_QUESTION] = "質問";
          cpt_array[CAPTION_INPUT] = "入力クエリ";
          cpt_array[CAPTION_OPEN] = "開ける";
          cpt_array[CAPTION_SAVE] = "名前を付けて保存";
          cpt_array[CAPTION_DIRECTORY] = "ディレクトリを選択";
          cpt_array[CAPTION_COLOR] = "色";
          cpt_array[CAPTION_ERROR] = "エラー";

        } else if (ptr[0] == 'k' && ptr[1] == 'o') {
          // ko = Korean

          btn_array[BUTTON_ABORT] = "중단";
          btn_array[BUTTON_IGNORE] = "무시하다";
          btn_array[BUTTON_OK] = "좋아요";
          btn_array[BUTTON_CANCEL] = "취소";
          btn_array[BUTTON_YES] = "예";
          btn_array[BUTTON_NO] = "아니요";
          btn_array[BUTTON_RETRY] = "다시 해 보다";

          cpt_array[CAPTION_INFORMATION] = "정보";
          cpt_array[CAPTION_QUESTION] = "질문";
          cpt_array[CAPTION_INPUT] = "입력 쿼리";
          cpt_array[CAPTION_OPEN] = "열려 있는";
          cpt_array[CAPTION_SAVE] = "다른 이름으로 저장";
          cpt_array[CAPTION_DIRECTORY] = "디렉터리 선택";
          cpt_array[CAPTION_COLOR] = "색상";
          cpt_array[CAPTION_ERROR] = "오류";

        } else if (ptr[0] == 'r' && ptr[1] == 'u') {
          // ru = Russian

          btn_array[BUTTON_ABORT] = "Отмена";
          btn_array[BUTTON_IGNORE] = "Игнорировать";
          btn_array[BUTTON_OK] = "ХОРОШО";
          btn_array[BUTTON_CANCEL] = "Отмена";
          btn_array[BUTTON_YES] = "Да";
          btn_array[BUTTON_NO] = "Нет";
          btn_array[BUTTON_RETRY] = "Повторить попытку";

          cpt_array[CAPTION_INFORMATION] = "Информация";
          cpt_array[CAPTION_QUESTION] = "Вопрос";
          cpt_array[CAPTION_INPUT] = "Входной запрос";
          cpt_array[CAPTION_OPEN] = "Открыть";
          cpt_array[CAPTION_SAVE] = "Сохранить как";
          cpt_array[CAPTION_DIRECTORY] = "Выберите каталог";
          cpt_array[CAPTION_COLOR] = "Цвет";
          cpt_array[CAPTION_ERROR] = "Ошибка";

        } else if (ptr[0] == 'p' && ptr[1] == 't') {
          // pt = Portuguese

          btn_array[BUTTON_ABORT] = "Abortar";
          btn_array[BUTTON_IGNORE] = "Ignorar";
          btn_array[BUTTON_OK] = "OK";
          btn_array[BUTTON_CANCEL] = "Cancelar";
          btn_array[BUTTON_YES] = "Sim";
          btn_array[BUTTON_NO] = "Não";
          btn_array[BUTTON_RETRY] = "Tentar novamente";

          cpt_array[CAPTION_INFORMATION] = "Informação";
          cpt_array[CAPTION_QUESTION] = "Pergunta";
          cpt_array[CAPTION_INPUT] = "Consulta de entrada";
          cpt_array[CAPTION_OPEN] = "Abrir";
          cpt_array[CAPTION_SAVE] = "Salvar como";
          cpt_array[CAPTION_DIRECTORY] = "Selecionar diretório";
          cpt_array[CAPTION_COLOR] = "Cor";
          cpt_array[CAPTION_ERROR] = "Erro";

        } else if (ptr[0] == 'a' && ptr[1] == 'r') {
          // ar = Arabic

          btn_array[BUTTON_ABORT] = "إلغاء";
          btn_array[BUTTON_IGNORE] = "يتجاهل";
          btn_array[BUTTON_OK] = "نعم";
          btn_array[BUTTON_CANCEL] = "يلغي";
          btn_array[BUTTON_YES] = "نعم";
          btn_array[BUTTON_NO] = "لا";
          btn_array[BUTTON_RETRY] = "إعادة المحاولة";

          cpt_array[CAPTION_INFORMATION] = "معلومة";
          cpt_array[CAPTION_QUESTION] = "سؤال";
          cpt_array[CAPTION_INPUT] = "استعلام الإدخال";
          cpt_array[CAPTION_OPEN] = "يفتح";
          cpt_array[CAPTION_SAVE] = "حفظ باسم";
          cpt_array[CAPTION_DIRECTORY] = "اختر المجلد";
          cpt_array[CAPTION_COLOR] = "لون";
          cpt_array[CAPTION_ERROR] = "خطأ";

        } else if (ptr[0] == 'h' && ptr[1] == 'i') {
          // hi = Hindi

          btn_array[BUTTON_ABORT] = "बीच में बंद करें";
          btn_array[BUTTON_IGNORE] = "अनदेखा करना";
          btn_array[BUTTON_OK] = "ठीक है";
          btn_array[BUTTON_CANCEL] = "रद्द करना";
          btn_array[BUTTON_YES] = "हाँ";
          btn_array[BUTTON_NO] = "नहीं";
          btn_array[BUTTON_RETRY] = "पुन: प्रयास करें";

          cpt_array[CAPTION_INFORMATION] = "जानकारी";
          cpt_array[CAPTION_QUESTION] = "सवाल";
          cpt_array[CAPTION_INPUT] = "इनपुट क्वेरी";
          cpt_array[CAPTION_OPEN] = "खुला";
          cpt_array[CAPTION_SAVE] = "के रूप रक्षित करें";
          cpt_array[CAPTION_DIRECTORY] = "डायरेक्टरी चुनें";
          cpt_array[CAPTION_COLOR] = "रंग";
          cpt_array[CAPTION_ERROR] = "गलती";

        } else if (ptr[0] == 'i' && ptr[1] == 't') {
          // it = Italian

          btn_array[BUTTON_ABORT] = "Interrompi";
          btn_array[BUTTON_IGNORE] = "Ignorare";
          btn_array[BUTTON_OK] = "OK";
          btn_array[BUTTON_CANCEL] = "Cancellare";
          btn_array[BUTTON_YES] = "SÌ";
          btn_array[BUTTON_NO] = "NO";
          btn_array[BUTTON_RETRY] = "Riprova";

          cpt_array[CAPTION_INFORMATION] = "Informazioni";
          cpt_array[CAPTION_QUESTION] = "Domanda";
          cpt_array[CAPTION_INPUT] = "Query di input";
          cpt_array[CAPTION_OPEN] = "Aprire";
          cpt_array[CAPTION_SAVE] = "Salva con nome";
          cpt_array[CAPTION_DIRECTORY] = "Seleziona directory";
          cpt_array[CAPTION_COLOR] = "Colore";
          cpt_array[CAPTION_ERROR] = "Errore";

        } else if (ptr[0] == 'n' && ptr[1] == 'l') {
          // nl = Dutch

          btn_array[BUTTON_ABORT] = "Afbreken";
          btn_array[BUTTON_IGNORE] = "Negeren";
          btn_array[BUTTON_OK] = "OK";
          btn_array[BUTTON_CANCEL] = "Annuleren";
          btn_array[BUTTON_YES] = "Ja";
          btn_array[BUTTON_NO] = "Nee";
          btn_array[BUTTON_RETRY] = "Opnieuw proberen";

          cpt_array[CAPTION_INFORMATION] = "Informatie";
          cpt_array[CAPTION_QUESTION] = "Vraag";
          cpt_array[CAPTION_INPUT] = "Invoervraag";
          cpt_array[CAPTION_OPEN] = "Open";
          cpt_array[CAPTION_SAVE] = "Opslaan als";
          cpt_array[CAPTION_DIRECTORY] = "Map selecteren";
          cpt_array[CAPTION_COLOR] = "Kleur";
          cpt_array[CAPTION_ERROR] = "Fout";

        }
      }
    }

    wstring widen(string str) {
      if (str.empty()) return L"";
      size_t wchar_count = str.size() + 1;
      vector<wchar_t> buf(wchar_count);
      wchar_count = (size_t)MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), (int)wchar_count);
      if (!wchar_count) return L"";
      return wstring { buf.data(), wchar_count };
    }

    string narrow(wstring wstr) {
      if (wstr.empty()) return "";
      int nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);
      if (!nbytes) return "";
      vector<char> buf((size_t)nbytes);
      nbytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), buf.data(), nbytes, nullptr, nullptr);
      if (!nbytes) return "";
      return string { buf.data(), (size_t)nbytes };
    }

    wchar_t *_wrealpath(const wchar_t *path, wchar_t *resolved_path) {
      wstring result;
      wchar_t buf[MAX_PATH];
      wchar_t *ptr = (((wchar_t *)resolved_path) ? ((wchar_t *)resolved_path) : ((wchar_t *)buf));
      HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
      if (hFile != INVALID_HANDLE_VALUE) {
        unsigned long len = GetFinalPathNameByHandleW(hFile, ptr, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (len && len <= MAX_PATH - 1) {
          result = ptr;
          if (!result.substr(0, 8).compare(L"\\\\?\\UNC\\")) {
            result = L"\\" + result.substr(7);
          } else if (!result.substr(0, 4).compare(L"\\\\?\\")) {
            result = result.substr(4);
          }
        }
        CloseHandle(hFile);
      }
      if (!result.empty()) {
        if (!resolved_path) {
          return _wcsdup(result.c_str());
        } else {
          wcsncpy_s(ptr, MAX_PATH, result.c_str(), _TRUNCATE);
          return (wchar_t *)ptr;
        }
      }
      return nullptr;
    }

    string string_replace_all(string str, string substr, string newstr) {
      size_t pos = 0;
      const size_t sublen = substr.length(), newlen = newstr.length();
      while ((pos = str.find(substr, pos)) != string::npos) {
        str.replace(pos, sublen, newstr);
        pos += newlen;
      }
      return str;
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
      switch (msg) {
        case WM_DESTROY:
          return 0;
        default:
          return DefWindowProc(hWnd, msg, wParam, lParam);
      }
    }

    HWND owner_window() {
      hwnds.clear();
      win = owner ? (HWND)owner : GetForegroundWindow();
      win = (unsigned long long)win ? win : GetDesktopWindow();
      if (parent) return parent;
      WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
      wc.lpfnWndProc = WndProc;
      wc.lpszClassName = L"OwnerWindow";
      wc.hInstance = GetModuleHandleW(nullptr);
      if (!RegisterClassExW(&wc)) return nullptr;
      HWND ret = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"", WS_VISIBLE | WS_POPUP,
      0, 0, 0, 0, win, nullptr, nullptr, nullptr);
      SetParent(parent, win);
      ShowWindow(parent, SW_SHOW);
      UpdateWindow(parent);
      parent = ret;
      return ret;
    }

    int show_message_helper(const char *str, bool cancelable) {
      string tstr = str; wstring wstr = widen(tstr);

      string title = (caption == "") ? (cancelable ? cpt_array[CAPTION_QUESTION] : cpt_array[CAPTION_INFORMATION]) : caption;
      wstring wtitle = widen(title);

      UINT flags = MB_DEFBUTTON1 | MB_APPLMODAL;
      flags |= cancelable ? (MB_OKCANCEL | MB_ICONQUESTION) : (MB_OK | MB_ICONINFORMATION);

      HWND o = owner_window();
      int result = MessageBoxW(o, wstr.c_str(), wtitle.c_str(), flags);
      return cancelable ? ((result == IDOK) ? 1 : -1) : 1;
    }

    int show_question_helper(const char *str, bool cancelable) {
      string tstr = str; wstring wstr = widen(tstr);

      string title = (caption == "") ? cpt_array[CAPTION_QUESTION] : caption;
      wstring wtitle = widen(title);

      UINT flags = MB_DEFBUTTON1 | MB_APPLMODAL | MB_ICONQUESTION;
      flags |= cancelable ? MB_YESNOCANCEL : MB_YESNO;

      HWND o = owner_window();
      int result = MessageBoxW(o, wstr.c_str(), wtitle.c_str(), flags);
      return cancelable ? ((result == IDYES) ? 1 : ((result == IDNO) ? 0 : -1)) : (result == IDYES);
    }

    int show_error_helper(const char *str, bool abort, bool attempt) {
      string tstr = str; wstring wstr = widen(tstr);

      string title = (caption == "") ? cpt_array[CAPTION_ERROR] : caption;
      wstring wtitle = widen(title);

      if (attempt) {
        UINT flags = MB_RETRYCANCEL | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL;
        HWND o = owner_window();
        int result = MessageBoxW(o, wstr.c_str(), wtitle.c_str(), flags);
        return (result == IDRETRY) ? 0 : -1;
      }

      UINT flags = (abort ? MB_OK : MB_OKCANCEL) | MB_ICONERROR | MB_DEFBUTTON1 | MB_APPLMODAL;
      HWND o = owner_window();
      int result = MessageBoxW(o, wstr.c_str(), wtitle.c_str(), flags);
      result = abort ? 1 : ((result == IDOK) ? 1 : -1);

      if (result == 1) exit(0);
      return result;
    }


    #ifdef _MSC_VER
    class CSimpleScriptSite :
      public IActiveScriptSite,
      public IActiveScriptSiteWindow {
    public:
      CSimpleScriptSite() : m_cRefCount(1), m_hWnd(nullptr) { }

      // IUnknown

      STDMETHOD_(ULONG, AddRef)();
      STDMETHOD_(ULONG, Release)();
      STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);

      // IActiveScriptSite

      STDMETHOD(GetLCID)(LCID *plcid) { *plcid = 0; return S_OK; }
      STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti) { return TYPE_E_ELEMENTNOTFOUND; }
      STDMETHOD(GetDocVersionString)(BSTR *pbstrVersion) { *pbstrVersion = SysAllocString(L"1.0"); return S_OK; }
      STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo) { return S_OK; }
      STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState) { return S_OK; }
      STDMETHOD(OnScriptError)(IActiveScriptError *pIActiveScriptError) { return S_OK; }
      STDMETHOD(OnEnterScript)(void) { return S_OK; }
      STDMETHOD(OnLeaveScript)(void) { return S_OK; }

      // IActiveScriptSiteWindow

      STDMETHOD(GetWindow)(HWND *phWnd) { *phWnd = m_hWnd; return S_OK; }
      STDMETHOD(EnableModeless)(BOOL fEnable) { return S_OK; }

      // Miscellaneous

      STDMETHOD(SetWindow)(HWND hWnd) { m_hWnd = hWnd; return S_OK; }

    public:
      LONG m_cRefCount;
      HWND m_hWnd;
    };

    STDMETHODIMP_(ULONG) CSimpleScriptSite::AddRef() {
      return InterlockedIncrement(&m_cRefCount);
    }

    STDMETHODIMP_(ULONG) CSimpleScriptSite::Release() {
      if (!InterlockedDecrement(&m_cRefCount)) {
        delete this;
        return 0;
      }
      return m_cRefCount;
    }

    STDMETHODIMP CSimpleScriptSite::QueryInterface(REFIID riid, void **ppvObject) {
      if (riid == IID_IUnknown || riid == IID_IActiveScriptSiteWindow) {
        *ppvObject = (IActiveScriptSiteWindow *)this;
        AddRef();
        return NOERROR;
      }
      if (riid == IID_IActiveScriptSite) {
        *ppvObject = (IActiveScriptSite *)this;
        AddRef();
        return NOERROR;
      }
      return E_NOINTERFACE;
    }
    #endif

    HICON GetIcon(HWND hwnd) {
      HICON icon = (HICON)SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0);
      if (icon == nullptr)
        icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
      if (icon == nullptr)
        icon = LoadIcon(GetModuleHandleW(nullptr), MAKEINTRESOURCE(0));
      if (icon == nullptr)
        icon = LoadIcon(nullptr, IDI_APPLICATION);
      return icon;
    }

    UINT_PTR CALLBACK GetColorProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
      if (uiMsg == WM_INITDIALOG) {
        cancel_pressed = false;
        if (tstr_gctitle != "")
          SetWindowTextW(hdlg, cpp_wstr_gctitle.c_str());
        PostMessageW(hdlg, WM_SETFOCUS, 0, 0);
      }
      return false;
    }

    #ifdef _MSC_VER
    LRESULT CALLBACK InputBoxProc(int nCode, WPARAM wParam, LPARAM lParam) {
      if (nCode < HC_ACTION)
        return CallNextHookEx(hhook, nCode, wParam, lParam);

      if (nCode == HCBT_CREATEWND) {
        CBT_CREATEWNDW *cbtcr = (CBT_CREATEWNDW *)lParam;
        if (cbtcr->lpcs->hwndParent == parent) {
          hwnds.push_back((HWND)wParam);
        }
      }
      
      for (int i = 0; i < hwnds.size(); i++) {
        dlg = hwnds[i];
        if (IsWindow(dlg)) {
          if (hidden == true) {
            SendDlgItemMessageW(dlg, 1000, EM_SETPASSWORDCHAR, L'\x25cf', 0);
          }
          wstring cpp_wstr_icon = widen(tstr_icon);
          if (PathFileExistsW(cpp_wstr_icon.c_str())) {
            HICON hIcon;
            ULONG_PTR m_gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
            Bitmap *png = Bitmap::FromFile(cpp_wstr_icon.c_str());
            png->GetHICON(&hIcon);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            delete png;
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
          } else {
            HICON hIcon = GetIcon(win);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
          }
        }
      }
      if (nCode == HCBT_SETFOCUS) {
        POINT pt;
        if (GetCursorPos(&pt) && ScreenToClient(dlg, &pt) && 
          GetDlgItem(dlg, 2) == ChildWindowFromPoint(dlg, pt)) {
          cancel_pressed = true;
        } else {
          cancel_pressed = false;
        }
      }
      return CallNextHookEx(hhook, nCode, wParam, lParam);
    }
    #endif

    LRESULT CALLBACK MessageBoxProc(int nCode, WPARAM wParam, LPARAM lParam) {
      if (nCode < HC_ACTION)
        return CallNextHookEx(hhook, nCode, wParam, lParam);

      if (nCode == HCBT_CREATEWND) {
        CBT_CREATEWNDW *cbtcr = (CBT_CREATEWNDW *)lParam;
        if (cbtcr->lpcs->hwndParent == parent) {
          hwnds.push_back((HWND)wParam);
        }
      }
      
      for (int i = 0; i < hwnds.size(); i++) {
        dlg = hwnds[i];
        if (IsWindow(dlg)) {
          wstring wstr_ok = widen(btn_array[BUTTON_OK]);
          wstring wstr_yes = widen(btn_array[BUTTON_YES]);
          wstring wstr_no = widen(btn_array[BUTTON_NO]);
          wstring wstr_retry = widen(btn_array[BUTTON_RETRY]);
          wstring wstr_cancel = widen(btn_array[BUTTON_CANCEL]);
          SetDlgItemTextW(dlg, IDOK, wstr_ok.c_str());
          SetDlgItemTextW(dlg, IDYES, wstr_yes.c_str());
          SetDlgItemTextW(dlg, IDNO, wstr_no.c_str());
          SetDlgItemTextW(dlg, IDRETRY, wstr_retry.c_str());
          SetDlgItemTextW(dlg, IDCANCEL, wstr_cancel.c_str());
          wstring cpp_wstr_icon = widen(tstr_icon);
          if (PathFileExistsW(cpp_wstr_icon.c_str())) {
            HICON hIcon;
            ULONG_PTR m_gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
            Bitmap *png = Bitmap::FromFile(cpp_wstr_icon.c_str());
            png->GetHICON(&hIcon);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            delete png;
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
          } else {
            HICON hIcon = GetIcon(win);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
          }
        }
      }
      return CallNextHookEx(hhook, nCode, wParam, lParam);
    }

    LRESULT CALLBACK ShowErrorProc(int nCode, WPARAM wParam, LPARAM lParam) {
      if (nCode < HC_ACTION)
        return CallNextHookEx(hhook, nCode, wParam, lParam);

      if (nCode == HCBT_CREATEWND) {
        CBT_CREATEWNDW *cbtcr = (CBT_CREATEWNDW *)lParam;
        if (cbtcr->lpcs->hwndParent == parent) {
          hwnds.push_back((HWND)wParam);
        }
      }
      
      for (int i = 0; i < hwnds.size(); i++) {
        dlg = hwnds[i];
        if (IsWindow(dlg)) {
          wstring wstr_abort = widen(btn_array[BUTTON_ABORT]);
          wstring wstr_ignore = widen(btn_array[BUTTON_IGNORE]);
          SetDlgItemTextW(dlg, IDOK, wstr_abort.c_str());
          SetDlgItemTextW(dlg, IDCANCEL, wstr_ignore.c_str());
          wstring cpp_wstr_icon = widen(tstr_icon);
          if (PathFileExistsW(cpp_wstr_icon.c_str())) {
            HICON hIcon;
            ULONG_PTR m_gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
            Bitmap *png = Bitmap::FromFile(cpp_wstr_icon.c_str());
            png->GetHICON(&hIcon);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            delete png;
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
          } else {
            HICON hIcon = GetIcon(win);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
          }
        }
      }
      return CallNextHookEx(hhook, nCode, wParam, lParam);
    }

    LRESULT CALLBACK DialogProc(int nCode, WPARAM wParam, LPARAM lParam) {
      if (nCode < HC_ACTION)
        return CallNextHookEx(hhook, nCode, wParam, lParam);

      if (nCode == HCBT_CREATEWND) {
        CBT_CREATEWNDW *cbtcr = (CBT_CREATEWNDW *)lParam;
        if (cbtcr->lpcs->hwndParent == parent) {
          hwnds.push_back((HWND)wParam);
        }
      }
      
      for (int i = 0; i < hwnds.size(); i++) {
        dlg = hwnds[i];
        if (IsWindow(dlg)) {
          wstring cpp_wstr_icon = widen(tstr_icon);
          if (PathFileExistsW(cpp_wstr_icon.c_str())) {
            HICON hIcon;
            ULONG_PTR m_gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
            Bitmap *png = Bitmap::FromFile(cpp_wstr_icon.c_str());
            png->GetHICON(&hIcon);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            delete png;
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
          } else {
            HICON hIcon = GetIcon(win);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
          }
        }
      }
      return CallNextHookEx(hhook, nCode, wParam, lParam);
    }

    vector<string> string_split(string str, char delimiter) {
      vector<string> vec;
      stringstream sstr(str);
      string tmp;
      while (std::getline(sstr, tmp, delimiter)) {
        vec.push_back(tmp);
      }
      return vec;
    }

    string filename_change_ext(string fname, string newext) {
      size_t fp = fname.find_last_of(".");
      if (fp == string::npos) return fname + newext;
      return fname.replace(fp, fname.length(), newext);
    }

    string filename_name(string fname) {
      size_t fp = fname.find_last_of("\\/");
      if (fp == string::npos) return fname;
      return fname.substr(fp + 1);
    }

    string filename_ext(string fname) {
      fname = filename_name(fname);
      size_t fp = fname.find_last_of(".");
      if (fp == string::npos) return "";
      return fname.substr(fp);
    }

    LRESULT CALLBACK SaveAsProc(int nCode, WPARAM wParam, LPARAM lParam) {
      if (nCode < HC_ACTION)
        return CallNextHookEx(hhook, nCode, wParam, lParam);

      if (nCode == HCBT_CREATEWND) {
        CBT_CREATEWNDW *cbtcr = (CBT_CREATEWNDW *)lParam;
        if (cbtcr->lpcs->hwndParent == parent) {
          hwnds.push_back((HWND)wParam);
        }
      }
      
      for (int i = 0; i < hwnds.size(); i++) {
        dlg = hwnds[i];
        if (IsWindow(dlg)) {
          textbox = FindWindowEx(dlg, nullptr, "DUIViewWndClassName", nullptr);
          textbox = FindWindowEx(textbox, nullptr, "DirectUIHWND", nullptr);
          textbox = FindWindowEx(textbox, nullptr, "FloatNotifySink", nullptr);
          textbox = FindWindowEx(textbox, nullptr, "ComboBox", nullptr);
          textbox = FindWindowEx(textbox, nullptr, "Edit", nullptr);
          wchar_t *textstr = new wchar_t[MAX_PATH]();
          GetWindowTextW(textbox, textstr, MAX_PATH);
          bool equalsext = false;
          vector<string> pipesplit = string_split(narrow(pipefilter), '|');
          for (unsigned i = 0; i < pipesplit.size(); i++) {
            if (i % 2 != 0) {
              vector<string> semicolonsplit = string_split(pipesplit[i], ';');
              for (int j = 0; j < semicolonsplit.size(); j++) {
                semicolonsplit[j] = string_replace_all(semicolonsplit[j], "*.*", "");
                semicolonsplit[j] = string_replace_all(semicolonsplit[j], "*", "");
                if (filename_ext(narrow(textstr)) == semicolonsplit[j]) {
                  equalsext = true;
                }
              }
              if (!equalsext && filename_ext(narrow(textstr)) == "" && semicolonsplit.size()) {
                wstring textwstr = widen(filename_change_ext(narrow(textstr), semicolonsplit[0]));
                SetWindowTextW(textbox, textwstr.c_str());
              }
            }
          }
          delete[] textstr; 
          textstr = nullptr;
          wstring cpp_wstr_icon = widen(tstr_icon);
          if (PathFileExistsW(cpp_wstr_icon.c_str())) {
            HICON hIcon;
            ULONG_PTR m_gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
            Bitmap *png = Bitmap::FromFile(cpp_wstr_icon.c_str());
            png->GetHICON(&hIcon);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            delete png;
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
          } else {
            HICON hIcon = GetIcon(win);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
          }
        }
      }
      return CallNextHookEx(hhook, nCode, wParam, lParam);
    }

    #ifndef _MSC_VER
    string InputBoxResult;
    std::vector<HWND> windows_from_proc_id(apiprocess::proc_id_t proc_id) {
      std::vector<HWND> windows;
      HWND hWnd = GetTopWindow(GetDesktopWindow());
      apiprocess::proc_id_t pid = 0; GetWindowThreadProcessId(hWnd, &pid);
      if (proc_id == pid) windows.push_back(hWnd);
      while ((hWnd = GetWindow(hWnd, GW_HWNDNEXT))) {
        apiprocess::proc_id_t pid = 0; GetWindowThreadProcessId(hWnd, &pid);
        if (proc_id == pid) windows.push_back(hWnd);
      }
      return windows;
    }
    #endif
    const char *InputBox(const char *Prompt, const char *Title, const char *Default) {
      HWND o = owner_window();
      #ifdef _MSC_VER
      HRESULT hr = S_OK;
      hr = CoInitialize(nullptr);

      // Initialize
      CSimpleScriptSite *pScriptSite = new CSimpleScriptSite();
      CComPtr<IActiveScript> spVBScript;
      CComPtr<IActiveScriptParse> spVBScriptParse;
      #endif
      HWND parent_window = o;
      #ifdef _MSC_VER
      hr = pScriptSite->SetWindow(parent_window);
      hr = spVBScript.CoCreateInstance(OLESTR("VBScript"));
      hr = spVBScript->SetScriptSite(pScriptSite);
      hr = spVBScript->QueryInterface(&spVBScriptParse);
      hr = spVBScriptParse->InitNew();
      #endif

      // Replace quotes with double quotes
      string strPrompt = string_replace_all(Prompt, "\"", "\"\"");
      string strTitle = string_replace_all(Title, "\"", "\"\"");
      #ifdef _MSC_VER
      string strDefault = string_replace_all(Default, "\"", "\"\"");
      #else
      string strDefault = ((!hidden) ? string_replace_all(Default, "\"", "\"\"") : "");
      wstring wstrDefault = widen(Default);
      #endif

      // Create evaluation string
      string Evaluation = "InputBox(\"" + strPrompt + "\", \"" + strTitle + "\", \"" + strDefault + "\")";
      Evaluation = string_replace_all(Evaluation, "\r", "");
      Evaluation = string_replace_all(Evaluation, "\n", "\" + vbNewLine + \"");
      wstring WideEval = widen(Evaluation);

      #ifdef _MSC_VER
      // Run InpuBox
      CComVariant result;
      EXCEPINFO ei = { };
      #endif

      #ifdef _MSC_VER
      DWORD ThreadID = GetCurrentThreadId();
      HINSTANCE ModHwnd = GetModuleHandle(nullptr);
      hhook = SetWindowsHookEx(WH_CBT, &InputBoxProc, ModHwnd, ThreadID);
      hr = spVBScriptParse->ParseScriptText(WideEval.c_str(), nullptr, nullptr, nullptr, 0, 0, SCRIPTTEXT_ISEXPRESSION, &result, &ei);
      UnhookWindowsHookEx(hhook);
      #else
      FILE *fp = nullptr;
      wchar_t wtemp[MAX_PATH];
      DWORD len = GetTempPathW(MAX_PATH, wtemp);
      if (!len || len > MAX_PATH - strlen("temp.XXXXXX.vbs")) {
        return "";
      }
      wstring wfname = wstring(wtemp) + L"temp.XXXXXX";
      wchar_t *wbuff = wfname.data(); if (_wmktemp_s(wbuff, wfname.length() + 1)) {
        return "";
      }
      if (_wfopen_s(&fp, wbuff, L"wb, ccs=UTF-8" )) {
        return "";
      }
      if (!fp) { return ""; }
      Evaluation = "WScript.Echo " + Evaluation;
      std::size_t result = fwrite(Evaluation.data(), sizeof(char), Evaluation.length(), fp);
      if (result < Evaluation.length()) { fclose(fp); return ""; }
      else { fclose(fp); }
      MoveFileW(wbuff, (wbuff + wstring(L".vbs")).c_str());
      apiprocess::proc_id_t proc_id = apiprocess::spawn_child_proc_id((string("cscript.exe /nologo \"") + narrow(wbuff) + string(".vbs\"")).c_str(), false);
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::vector<HWND> wins = windows_from_proc_id(proc_id);
      for (int i = 0; i < wins.size(); i++) {
        HWND dlg = wins[i];
        if (IsWindow(dlg)) {
          SetWindowLongPtr(dlg, GWLP_HWNDPARENT, (LONG_PTR)o);
          POINT pt;
          if (GetCursorPos(&pt) && ScreenToClient(dlg, &pt) && 
            GetDlgItem(dlg, 2) == ChildWindowFromPoint(dlg, pt)) {
            cancel_pressed = true;
          } else {
            cancel_pressed = false;
          }
          if (hidden == true) {
            SendDlgItemMessageW(dlg, 1000, WM_SETTEXT, 0, (LPARAM)wstrDefault.c_str());
            SendDlgItemMessageW(dlg, 1000, EM_SETPASSWORDCHAR, L'\x25cf', 0);
            SendDlgItemMessageW(dlg, 1000, WM_LBUTTONDOWN, 0, 0);
          }
          wstring cpp_wstr_icon = widen(tstr_icon);
          if (PathFileExistsW(cpp_wstr_icon.c_str())) {
            HICON hIcon;
            ULONG_PTR m_gdiplusToken;
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
            Bitmap *png = Bitmap::FromFile(cpp_wstr_icon.c_str());
            png->GetHICON(&hIcon);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            delete png;
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
          } else {
            HICON hIcon = GetIcon(win);
            PostMessage(dlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
          }
        }
      }
      EnableWindow(o, false);
      while (proc_id != 0 && !apiprocess::child_proc_id_is_complete(proc_id)) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }  
      }
      InputBoxResult.clear();
      InputBoxResult = apiprocess::read_from_stdout_for_child_proc_id(proc_id);
      while (!InputBoxResult.empty() && (InputBoxResult.back() == ' ' || 
        InputBoxResult.back() == '\t' || InputBoxResult.back() == '\r' || InputBoxResult.back() == '\n'))
        InputBoxResult.pop_back();
      static string strResult;
      strResult = InputBoxResult;
      apiprocess::free_stdout_for_child_proc_id(proc_id);
      apiprocess::free_stdin_for_child_proc_id(proc_id);
      DeleteFileW((wbuff + wstring(L".vbs")).c_str());
      EnableWindow(o, true);
      #endif
      #ifdef _MSC_VER
      // Cleanup
      spVBScriptParse = nullptr;
      spVBScript = nullptr;
      pScriptSite->Release();
      pScriptSite = nullptr;

      ::CoUninitialize();
      static string strResult;
      _bstr_t bstrResult = (_bstr_t)result;
      strResult = narrow((wchar_t *)bstrResult);
      #endif
      if (strResult.empty()) {
        cancel_pressed = true;
      }
      return strResult.c_str();
    }

    const char *get_string_helper(const char *str, const char *def, bool hide) {
      hidden = hide; string title = (caption == "") ? cpt_array[CAPTION_INPUT] : caption;
      return InputBox(str, title.c_str(), def);
    }

    string remove_trailing_zeros(double numb) {
      string strnumb = to_string(numb);

      while (!strnumb.empty() && strnumb.find('.') != string::npos && (strnumb.back() == '.' || strnumb.back() == '0'))
        strnumb.pop_back();

      return strnumb;
    }

    double get_integer_helper(const char *str, double def, bool hide) {
      double DIGITS_MIN = -999999999999999;
      double DIGITS_MAX = 999999999999999;

      if (def < DIGITS_MIN) def = DIGITS_MIN;
      if (def > DIGITS_MAX) def = DIGITS_MAX;

      string cpp_tdef = remove_trailing_zeros(def);
      double result = strtod(get_string_helper(str, cpp_tdef.c_str(), hide), nullptr);

      if (result < DIGITS_MIN) result = DIGITS_MIN;
      if (result > DIGITS_MAX) result = DIGITS_MAX;

      return result;
    }

    string remove_slash(string dir) {
      while (!dir.empty() && (dir.back() == '\\' || dir.back() == '/'))
        dir.pop_back();
      return dir;
    }

    string add_slash(string dir) {
      dir = remove_slash(dir);
      if (!dir.empty() && (dir.back() != '\\' && dir.back() != '/')) dir += '\\';
      return dir;
    }

    HWND owner_temp = nullptr;
    
    OPENFILENAMEW get_filename_or_filenames_helper(string filter, string fname, string dir, string title, DWORD flags) {
      filter = filter.append("||");
      fname = remove_slash(fname);

      wstring cpp_wstr_filter = widen(filter);
      wstring cpp_wstr_fname = widen(fname);
      cpp_wstr_dir = widen(dir);
      cpp_wstr_title = widen(title);
      pipefilter = cpp_wstr_filter;

      wstr_filter = new wchar_t[cpp_wstr_filter.length() + 1]();
      wcsncpy_s(wstr_filter, cpp_wstr_filter.length() + 1,
      cpp_wstr_filter.c_str(), cpp_wstr_filter.length() + 1);
      wcsncpy_s(wstr_fname, 32767, cpp_wstr_fname.c_str(), 32767);

      wstring defext;
      vector<string> pipesplit = string_split(narrow(pipefilter), '|');
      for (unsigned i = 0; i < pipesplit.size(); i++) {
        if (i % 2 != 0) {
          vector<string> semicolonsplit = string_split(pipesplit[i], ';');
          for (int j = 0; j < semicolonsplit.size(); j++) {
            semicolonsplit[j] = string_replace_all(semicolonsplit[j], "*.*", "");
            semicolonsplit[j] = string_replace_all(semicolonsplit[j], "*.", "");
            defext = widen(semicolonsplit[0]);
            break;
          }
        }
      }

      int i = 0;
      while (wstr_filter[i] != L'\0') {
        if (wstr_filter[i] == L'|') {
          wstr_filter[i] = L'\0';
        }
        i++;
      }

      owner_temp = owner_window();
      ZeroMemory(&ofn, sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = owner_temp;
      ofn.lpstrFile = wstr_fname;
      ofn.nMaxFile = 32767;
      ofn.lpstrFilter = wstr_filter;
      ofn.nMaxCustFilter = sizeof(wstr_filter) / sizeof(*wstr_filter);
      ofn.nFilterIndex = 0;
      ofn.lpstrDefExt = defext.c_str();
      ofn.lpstrTitle = cpp_wstr_title.c_str();
      ofn.lpstrInitialDir = cpp_wstr_dir.c_str();
      ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | flags;

      return ofn;
    }

    string get_open_filename_helper(string filter, string fname, string dir, string title) {
      cancel_pressed = false;
      ofn = get_filename_or_filenames_helper(filter, fname, dir, title, 0);

      if (GetOpenFileNameW(&ofn) != 0) {
        string result = narrow(wstr_fname);
        delete[] wstr_filter;
        wstr_filter = nullptr;
        return result;
      }
      return "";
    }
    
    string filename_path(string fname) {
      size_t fp = fname.find_last_of("\\/");
      if (fp == string::npos) return fname;
      return fname.substr(0, fp + 1);
    }

    string get_open_filenames_helper(string filter, string fname, string dir, string title) {
      cancel_pressed = false;
      files = ""; ofn = get_filename_or_filenames_helper(filter, fname, dir, title, OFN_ALLOWMULTISELECT);
      if (GetOpenFileNameW(&ofn) != 0) {
        size_t pos = 0, i = 0;
        while (wstr_fname[pos] != L'\0') {
          if (pos > 0) {
            files += add_slash(narrow(&wstr_fname[0])) +
            narrow(&wstr_fname[pos]) + "\n"; i++;
          }
          pos += wcslen(wstr_fname + pos) + 1;
        }
        if (!files.empty()) {
          files.pop_back();
        }
        if (i == 0) {
          files = narrow(&wstr_fname[0]);
        }
        delete[] wstr_filter;
        wstr_filter = nullptr;
        return files;
      }
      delete[] wstr_filter;
      wstr_filter = nullptr;
      return "";
    }

    string get_save_filename_helper(string filter, string fname, string dir, string title) {
      cancel_pressed = false;
      ofn = get_filename_or_filenames_helper(filter, fname, dir, title, OFN_OVERWRITEPROMPT);

      if (GetSaveFileNameW(&ofn) != 0) {
        string result = narrow(wstr_fname);
        delete[] wstr_filter;
        wstr_filter = nullptr;
        return result;
      }
      return "";
    }

    string get_directory_helper(string dname, string title) {
      cancel_pressed = false;
      HWND o = owner_window();
      cpp_wstr_title = widen(title);
      cpp_wstr_dir = (!dname.empty()) ? widen(dname) : L""; 
      IFileDialog *selectDirectory = nullptr;
      CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&selectDirectory));
      DWORD options = 0; selectDirectory->GetOptions(&options);
      selectDirectory->SetOptions(options | FOS_PICKFOLDERS | FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM);
      wchar_t *szFilePath = (wchar_t *)cpp_wstr_dir.c_str(); IShellItem *pItem = nullptr;
      HRESULT hr = SHCreateItemFromParsingName(szFilePath, nullptr, IID_PPV_ARGS(&pItem));
      if (SUCCEEDED(hr)) {
        wchar_t *szName = nullptr;
        hr = pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &szName);
        if (SUCCEEDED(hr)) {
          selectDirectory->SetFolder(pItem);
          CoTaskMemFree(szName);
        }
        pItem->Release();
      }
      selectDirectory->SetTitle(cpp_wstr_title.c_str());
      selectDirectory->Show(o); 
      pItem = nullptr;
      hr = selectDirectory->GetResult(&pItem);
      if (SUCCEEDED(hr)) {
        wchar_t *wstr_result = nullptr;
        pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &wstr_result);
        pItem->Release(); string str_result;
        str_result = add_slash(narrow(wstr_result));
        return str_result;
      }
      return "";
    }

    int get_color_helper(int defcol, string title) {
      CHOOSECOLORW cc;

      COLORREF DefColor = defcol;
      static COLORREF CustColors[16];

      tstr_gctitle = title;
      cpp_wstr_gctitle = widen(tstr_gctitle);

      HWND o = owner_window();
      ZeroMemory(&cc, sizeof(cc));
      cc.lStructSize = sizeof(CHOOSECOLORW);
      cc.hwndOwner = o;
      cc.rgbResult = DefColor;
      cc.lpCustColors = CustColors;
      cc.Flags = CC_RGBINIT | CC_ENABLEHOOK;
      cc.lpfnHook = GetColorProc;

      int ret = ((ChooseColorW(&cc) != 0) ? cc.rgbResult : -1);
      return ret;
    }

    void regain_focus_to_owner() {
      DWORD pid = 0;
      if (owner && IsWindow((HWND)owner)) {
        GetWindowThreadProcessId((HWND)owner, &pid);
        AllowSetForegroundWindow(pid);
        SetForegroundWindow((HWND)owner);
        SetActiveWindow((HWND)owner);
        SetFocus((HWND)owner);
      }
    }

  } // anonymous namespace

  int show_message(const char *str) {
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &MessageBoxProc, ModHwnd, ThreadID);
    static string cancel; cancel = widget_get_button_name(BUTTON_CANCEL);
    widget_set_button_name(BUTTON_CANCEL, widget_get_button_name(BUTTON_OK));
    int result = show_message_helper(str, false);
    widget_set_button_name(BUTTON_CANCEL, cancel.c_str());
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  int show_message_cancelable(const char *str) {
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &MessageBoxProc, ModHwnd, ThreadID);
    int result = show_message_helper(str, true);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  int show_question(const char *str) {
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &MessageBoxProc, ModHwnd, ThreadID);
    int result = show_question_helper(str, false);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  int show_question_cancelable(const char *str) {
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &MessageBoxProc, ModHwnd, ThreadID);
    int result = show_question_helper(str, true);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  int show_attempt(const char *str) {
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &MessageBoxProc, ModHwnd, ThreadID);
    int result = show_error_helper(str, false, true);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  int show_error(const char *str, bool abort) {
    fatal = abort;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &ShowErrorProc, ModHwnd, ThreadID);
    int result = -1;
    if (abort) {
      static string cancel; cancel = widget_get_button_name(BUTTON_CANCEL);
      widget_set_button_name(BUTTON_CANCEL, widget_get_button_name(BUTTON_OK));
      result = show_error_helper(str, abort, false);
      widget_set_button_name(BUTTON_CANCEL, cancel.c_str());
    } else {
      result = show_error_helper(str, abort, false);
    }
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  const char *get_string(const char *str, const char *def) {
    const char *result = get_string_helper(str, def, false);
    regain_focus_to_owner();
    return result;
  }

  const char *get_password(const char *str, const char *def) {
    const char *result = get_string_helper(str, def, true);
    regain_focus_to_owner();
    return result;
  }

  double get_integer(const char *str, double def) {
    double result = get_integer_helper(str, def, false);
    regain_focus_to_owner();
    return result;
  }

  double get_passcode(const char *str, double def) {
    double result = get_integer_helper(str, def, true);
    regain_focus_to_owner();
    return result;
  }

  const char *get_open_filename(const char *filter, const char *fname) {
    string str_filter = filter; string str_fname = fname; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    result = get_open_filename_helper(str_filter, str_fname, "", "");
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_open_filename_ext(const char *filter, const char *fname, const char *dir, const char *title) {
    string str_filter = filter; string str_fname = fname;
    string str_dir = dir; string str_title = title; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    result = get_open_filename_helper(str_filter, str_fname, str_dir, str_title);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_open_filenames(const char *filter, const char *fname) {
    string str_filter = filter; string str_fname = fname; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    result = get_open_filenames_helper(str_filter, str_fname, "", "");
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_open_filenames_ext(const char *filter, const char *fname, const char *dir, const char *title) {
    string str_filter = filter; string str_fname = fname;
    string str_dir = dir; string str_title = title; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    result = get_open_filenames_helper(str_filter, str_fname, str_dir, str_title);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_save_filename(const char *filter, const char *fname) {
    string str_filter = filter; string str_fname = fname; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &SaveAsProc, ModHwnd, ThreadID);
    result = get_save_filename_helper(str_filter, str_fname, "", "");
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_save_filename_ext(const char *filter, const char *fname, const char *dir, const char *title) {
    string str_filter = filter; string str_fname = fname;
    string str_dir = dir; string str_title = title; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &SaveAsProc, ModHwnd, ThreadID);
    result = get_save_filename_helper(str_filter, str_fname, str_dir, str_title);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_directory(const char *dname) {
    string str_dname = dname;  static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    result = get_directory_helper(str_dname, "");
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  const char *get_directory_alt(const char *capt, const char *root) {
    string str_dname = root; string str_title = capt; static string result;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    result = get_directory_helper(str_dname, str_title);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result.c_str();
  }

  int get_color(int defcol) {
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    int result = get_color_helper(defcol, "");
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  int get_color_ext(int defcol, const char *title) {
    string str_title = title;
    DWORD ThreadID = GetCurrentThreadId();
    HINSTANCE ModHwnd = GetModuleHandle(nullptr);
    hhook = SetWindowsHookEx(WH_CBT, &DialogProc, ModHwnd, ThreadID);
    int result = get_color_helper(defcol, str_title);
    UnhookWindowsHookEx(hhook);
    regain_focus_to_owner();
    return result;
  }

  const char *widget_get_caption() {
    return caption.c_str();
  }

  void widget_set_caption(const char *str) {
    caption = str;
  }

  const char *widget_get_owner() {
    static string hwnd;
    hwnd = std::to_string((unsigned long long)owner);
    return hwnd.c_str();
  }

  void widget_set_owner(const char *hwnd) {
    owner = (void *)strtoull(hwnd, nullptr, 10);
  }

  const char *widget_get_icon() {
    static string tstr_result;
    wchar_t wstr_icon[MAX_PATH];
    wstring cpp_wstr_icon = widen(tstr_icon);
    if (_wrealpath(cpp_wstr_icon.c_str(), wstr_icon)) {
      if (PathFileExistsW(wstr_icon)) {
        tstr_result = narrow(wstr_icon);
      }
    }
    return tstr_result.c_str();
  }

  void widget_set_icon(const char *icon) {
    wchar_t wstr_icon[MAX_PATH];
    wstring cpp_wstr_icon = widen(icon);
    if (_wrealpath(cpp_wstr_icon.c_str(), wstr_icon)) {
      if (PathFileExistsW(wstr_icon)) {
        tstr_icon = narrow(wstr_icon);
      }
    }
  }

  const char *widget_get_system() {
    return "Win32";
  }

  void widget_set_system(const char *sys) {

  }

  void widget_set_button_name(int type, const char *name) {
    string str_name = name;
    btn_array[type] = str_name;
  }

  const char *widget_get_button_name(int type) {
    return btn_array[type].c_str();
  }

  bool widget_get_canceled() {
    return cancel_pressed;
  }

  void widget_set_locale() {
    widget_set_locale_helper();
  }

} // namespace dialog_module
