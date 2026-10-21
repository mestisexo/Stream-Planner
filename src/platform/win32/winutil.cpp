#include "core/model.h"              // desde cualquier archivo
#include "platform/win32/app.h"      // dentro de platform/win32/
#include "platform/win32/winutil.h"

#include <cstdio>
#include <cstring>

void appendW(std::wstring& out, std::string_view s) {
    if (s.empty()) return;
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::size_t old = out.size();
    out.resize(old + n);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data() + old, n);
}

std::wstring W(std::string_view s) {
    std::wstring w;
    appendW(w, s);
    return w;
}

std::string U8(std::wstring_view w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring GetText(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n, L'\0');
    GetWindowTextW(h, s.data(), n + 1);  // el +1 es el terminador, permitido en s[size()]
    return s;
}

std::string today() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char b[16];
    snprintf(b, sizeof b, "%04d-%02d-%02d", t.wYear, t.wMonth, t.wDay);
    return b;
}

void warn(HWND owner, const std::wstring& msg) {
    MessageBoxW(owner, msg.c_str(), L"Stream Planner", MB_OK | MB_ICONWARNING);
}

void copyToClipboard(HWND owner, const std::wstring& s) {
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    HGLOBAL m = GlobalAlloc(GMEM_MOVEABLE, (s.size() + 1) * sizeof(wchar_t));
    if (m) {
        memcpy(GlobalLock(m), s.c_str(), (s.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(m);
        SetClipboardData(CF_UNICODETEXT, m);
    }
    CloseClipboard();
}
