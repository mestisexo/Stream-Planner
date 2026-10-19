// Stream Planner - interfaz Win32. Toda la lógica vive en core/model.*
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "core/model.h"

// Activa los controles con estilo moderno (Common Controls v6)
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

enum {
    IDC_SERIES = 100, IDC_NEWS, IDC_TOGGLE, IDC_DELS,
    IDC_EDNAME = 120, IDC_EDTITLE, IDC_EDNOTIF, IDC_SAVE, IDC_COPYT, IDC_COPYN,
    IDC_COL0 = 200,  // 200, 201, 202 = Planeado, En vivo, Hecho
    IDC_EDTOPIC = 210, IDC_EDDATE, IDC_ADD, IDC_LEFT, IDC_RIGHT, IDC_DELT
};

struct App {
    sp::Store store;
    std::filesystem::path dataPath;
    int curSeries = 0;
    int selStream = 0;
    bool loading = true;
    bool titleOver = false, notifOver = false;
    HWND hwnd = nullptr;
    HFONT font = nullptr;
    HWND lblSeries, lbSeries, btnNew, btnToggle, btnDelS;
    HWND lblName, edName, lblTitle, edTitle, stTitlePrev, stTitleCount;
    HWND lblNotif, edNotif, stNotifPrev, stNotifCount;
    HWND btnSave, btnCopyT, btnCopyN, stStats;
    HWND stCol[3], lbCol[3];
    HWND edTopic, edDate, btnAdd, btnLeft, btnRight, btnDelT;
} g;

// ---------- utilidades ----------
static std::wstring W(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}
static std::string U8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
static std::wstring GetText(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(h, &s[0], n + 1);
    s.resize(n);
    return s;
}
static std::string today() {
    SYSTEMTIME t;
    GetLocalTime(&t);
    char b[16];
    snprintf(b, sizeof b, "%04d-%02d-%02d", t.wYear, t.wMonth, t.wDay);
    return b;
}
static void warn(const std::wstring& msg) {
    MessageBoxW(g.hwnd, msg.c_str(), L"Stream Planner", MB_OK | MB_ICONWARNING);
}
static void persist() { g.store.save(g.dataPath); }
static sp::Series* cur() { return g.store.findSeries(g.curSeries); }

static void copyToClipboard(const std::wstring& s) {
    if (!OpenClipboard(g.hwnd)) return;
    EmptyClipboard();
    HGLOBAL m = GlobalAlloc(GMEM_MOVEABLE, (s.size() + 1) * sizeof(wchar_t));
    if (m) {
        memcpy(GlobalLock(m), s.c_str(), (s.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(m);
        SetClipboardData(CF_UNICODETEXT, m);
    }
    CloseClipboard();
}

// ---------- refrescos de UI ----------
static void refreshSeriesList() {
    SendMessageW(g.lbSeries, LB_RESETCONTENT, 0, 0);
    int sel = -1;
    for (auto& s : g.store.series) {
        std::wstring t = std::wstring(s.active ? L"[activa] " : L"[finalizada] ") + W(s.name);
        int i = (int)SendMessageW(g.lbSeries, LB_ADDSTRING, 0, (LPARAM)t.c_str());
        SendMessageW(g.lbSeries, LB_SETITEMDATA, i, s.id);
        if (s.id == g.curSeries) sel = i;
    }
    SendMessageW(g.lbSeries, LB_SETCURSEL, sel, 0);
}

static void refreshKanban() {
    for (int c = 0; c < 3; ++c) SendMessageW(g.lbCol[c], LB_RESETCONTENT, 0, 0);
    sp::Series* s = cur();
    if (!s) return;
    std::vector<const sp::Stream*> v;
    for (auto& t : g.store.streams)
        if (t.seriesId == s->id) v.push_back(&t);
    std::sort(v.begin(), v.end(), [](const sp::Stream* a, const sp::Stream* b) { return a->number < b->number; });
    for (const sp::Stream* t : v) {
        int c = (int)t->status;
        std::wstring txt = L"Día " + std::to_wstring(t->number) + L" - " + W(t->topic) + L"  (" + W(t->date) + L")";
        int i = (int)SendMessageW(g.lbCol[c], LB_ADDSTRING, 0, (LPARAM)txt.c_str());
        SendMessageW(g.lbCol[c], LB_SETITEMDATA, i, t->id);
        if (t->id == g.selStream) SendMessageW(g.lbCol[c], LB_SETCURSEL, i, 0);
    }
}

static void updateStats() {
    sp::Series* s = cur();
    if (!s) { SetWindowTextW(g.stStats, L"Crea o selecciona una serie."); return; }
    std::wstring t = L"Estado: " + std::wstring(s->active ? L"ACTIVA" : L"FINALIZADA") +
        L"   |   Vas en el día: " + std::to_wstring(g.store.lastRealized(s->id)) +
        L"   |   Hechos: " + std::to_wstring(g.store.count(s->id, sp::Status::Done)) +
        L"   |   Pendientes: " + std::to_wstring(g.store.count(s->id, sp::Status::Planned)) +
        L"   |   Próximo: día " + std::to_wstring(g.store.nextNumber(s->id));
    SetWindowTextW(g.stStats, t.c_str());
}

static void updatePreview() {
    sp::Series tmp;
    tmp.name = U8(GetText(g.edName));
    tmp.titleTemplate = U8(GetText(g.edTitle));
    tmp.notifTemplate = U8(GetText(g.edNotif));

    sp::Stream sample;
    const sp::Stream* st = g.store.findStream(g.selStream);
    if (!st) {
        sample.number = g.store.nextNumber(g.curSeries);
        sample.topic = "tema de ejemplo";
        sample.date = today();
        st = &sample;
    }
    std::string t = sp::render(tmp.titleTemplate, tmp, *st);
    std::string n = sp::render(tmp.notifTemplate, tmp, *st);
    size_t lt = sp::utf8Length(t), ln = sp::utf8Length(n);
    g.titleOver = lt > sp::kTwitchLimit;
    g.notifOver = ln > sp::kTwitchLimit;

    SetWindowTextW(g.stTitlePrev, W(t).c_str());
    SetWindowTextW(g.stNotifPrev, W(n).c_str());
    std::wstring ct = L"Título: " + std::to_wstring(lt) + L"/" + std::to_wstring(sp::kTwitchLimit) + (g.titleOver ? L"  ¡DEMASIADO LARGO!" : L"");
    std::wstring cn = L"Notificación: " + std::to_wstring(ln) + L"/" + std::to_wstring(sp::kTwitchLimit) + (g.notifOver ? L"  ¡DEMASIADO LARGA!" : L"");
    SetWindowTextW(g.stTitleCount, ct.c_str());
    SetWindowTextW(g.stNotifCount, cn.c_str());
    InvalidateRect(g.stTitleCount, nullptr, TRUE);
    InvalidateRect(g.stNotifCount, nullptr, TRUE);
}

static void loadFields() {
    g.loading = true;
    sp::Series* s = cur();
    SetWindowTextW(g.edName, s ? W(s->name).c_str() : L"");
    SetWindowTextW(g.edTitle, s ? W(s->titleTemplate).c_str() : L"");
    SetWindowTextW(g.edNotif, s ? W(s->notifTemplate).c_str() : L"");
    g.loading = false;
    refreshKanban();
    updatePreview();
    updateStats();
}

static void selectSeries(int id) {
    g.curSeries = id;
    g.selStream = 0;
    refreshSeriesList();
    loadFields();
}

// ---------- acciones ----------
static void actNewSeries() {
    int id = g.store.addSeries("Nueva serie");
    persist();
    selectSeries(id);
    SetFocus(g.edName);
    SendMessageW(g.edName, EM_SETSEL, 0, -1);
}

static void actSaveSeries() {
    sp::Series* s = cur();
    if (!s) return;
    if (g.titleOver || g.notifOver) {
        warn(L"El título o la notificación superan el límite de Twitch. Acórtalos antes de guardar.");
        return;
    }
    s->name = U8(GetText(g.edName));
    s->titleTemplate = U8(GetText(g.edTitle));
    s->notifTemplate = U8(GetText(g.edNotif));
    persist();
    refreshSeriesList();
    updatePreview();
}

static void actToggleActive() {
    sp::Series* s = cur();
    if (!s) return;
    s->active = !s->active;
    persist();
    refreshSeriesList();
    updateStats();
}

static void actDeleteSeries() {
    sp::Series* s = cur();
    if (!s) return;
    if (MessageBoxW(g.hwnd, L"¿Borrar la serie y todos sus streams?", L"Stream Planner",
                    MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    g.store.removeSeries(s->id);
    persist();
    selectSeries(g.store.series.empty() ? 0 : g.store.series.front().id);
}

static void actAddStream() {
    sp::Series* s = cur();
    if (!s) { warn(L"Primero crea una serie."); return; }
    std::string topic = U8(GetText(g.edTopic));
    std::string date = U8(GetText(g.edDate));
    if (date.empty()) date = today();

    // Validar contra el límite de Twitch usando la plantilla GUARDADA de la serie
    sp::Stream probe;
    probe.number = g.store.nextNumber(s->id);
    probe.topic = topic;
    probe.date = date;
    size_t lt = sp::utf8Length(sp::render(s->titleTemplate, *s, probe));
    size_t ln = sp::utf8Length(sp::render(s->notifTemplate, *s, probe));
    if (lt > sp::kTwitchLimit || ln > sp::kTwitchLimit) {
        warn(L"Con ese tema el título queda en " + std::to_wstring(lt) + L" caracteres y la notificación en " +
             std::to_wstring(ln) + L" (máximo " + std::to_wstring(sp::kTwitchLimit) + L"). Acorta el tema.");
        return;
    }
    g.selStream = g.store.addStream(s->id, topic, date);
    persist();
    SetWindowTextW(g.edTopic, L"");
    refreshKanban();
    updatePreview();
    updateStats();
}

static void actMoveStream(int dir) {
    sp::Stream* t = g.store.findStream(g.selStream);
    if (!t) return;
    int ns = (int)t->status + dir;
    if (ns < 0 || ns > 2) return;
    t->status = (sp::Status)ns;
    persist();
    refreshKanban();
    updateStats();
}

static void actDeleteStream() {
    if (!g.store.findStream(g.selStream)) return;
    g.store.removeStream(g.selStream);
    g.selStream = 0;
    persist();
    refreshKanban();
    updatePreview();
    updateStats();
}

// ---------- ventana ----------
static HWND mk(const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
    return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, g.hwnd,
                           (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
}

static BOOL CALLBACK setFontProc(HWND h, LPARAM f) {
    SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE);
    return TRUE;
}

static void createControls() {
    const DWORD LB = LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_BORDER;
    const DWORD ED = ES_AUTOHSCROLL | WS_BORDER;
    g.lblSeries = mk(L"STATIC", L"Series", 0, 0);
    g.lbSeries = mk(L"LISTBOX", L"", LB, IDC_SERIES);
    g.btnNew = mk(L"BUTTON", L"Nueva serie", BS_PUSHBUTTON, IDC_NEWS);
    g.btnToggle = mk(L"BUTTON", L"Activa / Finalizada", BS_PUSHBUTTON, IDC_TOGGLE);
    g.btnDelS = mk(L"BUTTON", L"Borrar serie", BS_PUSHBUTTON, IDC_DELS);

    g.lblName = mk(L"STATIC", L"Nombre de la serie", 0, 0);
    g.edName = mk(L"EDIT", L"", ED, IDC_EDNAME);
    g.lblTitle = mk(L"STATIC", L"Plantilla del título   (usa {series} {day} {topic} {date})", 0, 0);
    g.edTitle = mk(L"EDIT", L"", ED, IDC_EDTITLE);
    g.stTitlePrev = mk(L"STATIC", L"", 0, 0);
    g.stTitleCount = mk(L"STATIC", L"", 0, 0);
    g.lblNotif = mk(L"STATIC", L"Plantilla de la notificación a la comunidad", 0, 0);
    g.edNotif = mk(L"EDIT", L"", ED, IDC_EDNOTIF);
    g.stNotifPrev = mk(L"STATIC", L"", 0, 0);
    g.stNotifCount = mk(L"STATIC", L"", 0, 0);

    g.btnSave = mk(L"BUTTON", L"Guardar serie", BS_PUSHBUTTON, IDC_SAVE);
    g.btnCopyT = mk(L"BUTTON", L"Copiar título", BS_PUSHBUTTON, IDC_COPYT);
    g.btnCopyN = mk(L"BUTTON", L"Copiar notificación", BS_PUSHBUTTON, IDC_COPYN);
    g.stStats = mk(L"STATIC", L"", 0, 0);

    const wchar_t* heads[3] = {L"Planeado", L"En vivo", L"Hecho"};
    for (int c = 0; c < 3; ++c) {
        g.stCol[c] = mk(L"STATIC", heads[c], 0, 0);
        g.lbCol[c] = mk(L"LISTBOX", L"", LB, IDC_COL0 + c);
    }
    g.edTopic = mk(L"EDIT", L"", ED, IDC_EDTOPIC);
    g.edDate = mk(L"EDIT", W(today()).c_str(), ED, IDC_EDDATE);
    g.btnAdd = mk(L"BUTTON", L"Añadir stream", BS_PUSHBUTTON, IDC_ADD);
    g.btnLeft = mk(L"BUTTON", L"<", BS_PUSHBUTTON, IDC_LEFT);
    g.btnRight = mk(L"BUTTON", L">", BS_PUSHBUTTON, IDC_RIGHT);
    g.btnDelT = mk(L"BUTTON", L"Borrar", BS_PUSHBUTTON, IDC_DELT);

    SendMessageW(g.edTopic, EM_SETCUEBANNER, FALSE, (LPARAM)L"Tema del stream...");
    SendMessageW(g.edDate, EM_SETCUEBANNER, FALSE, (LPARAM)L"AAAA-MM-DD");

    g.font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    EnumChildWindows(g.hwnd, setFontProc, (LPARAM)g.font);
}

static void layout(int cw, int ch) {
    const int m = 10, lw = 250, x0 = lw + 2 * m, aw = std::max(300, cw - x0 - m);
    auto mv = [](HWND h, int x, int y, int w, int hh) { MoveWindow(h, x, y, w, hh, TRUE); };

    mv(g.lblSeries, m, 10, lw, 20);
    mv(g.lbSeries, m, 32, lw, ch - 32 - 110);
    mv(g.btnNew, m, ch - 100, lw, 28);
    mv(g.btnToggle, m, ch - 68, lw, 28);
    mv(g.btnDelS, m, ch - 36, lw, 28);

    mv(g.lblName, x0, 10, aw, 20);
    mv(g.edName, x0, 30, aw, 26);
    mv(g.lblTitle, x0, 62, aw, 20);
    mv(g.edTitle, x0, 82, aw, 26);
    mv(g.stTitlePrev, x0, 112, aw, 38);
    mv(g.stTitleCount, x0, 150, aw, 20);
    mv(g.lblNotif, x0, 176, aw, 20);
    mv(g.edNotif, x0, 196, aw, 26);
    mv(g.stNotifPrev, x0, 226, aw, 38);
    mv(g.stNotifCount, x0, 264, aw, 20);

    mv(g.btnSave, x0, 292, 140, 30);
    mv(g.btnCopyT, x0 + 148, 292, 140, 30);
    mv(g.btnCopyN, x0 + 296, 292, 180, 30);
    mv(g.stStats, x0, 330, aw, 22);

    int colW = (aw - 2 * m) / 3, top = 362, listH = std::max(60, ch - top - 22 - 54);
    for (int c = 0; c < 3; ++c) {
        int x = x0 + c * (colW + m);
        mv(g.stCol[c], x, top, colW, 20);
        mv(g.lbCol[c], x, top + 22, colW, listH);
    }
    int by = ch - 44;
    int topicW = std::max(100, aw - 440), x = x0;
    mv(g.edTopic, x, by, topicW, 28); x += topicW + 8;
    mv(g.edDate, x, by, 110, 28); x += 118;
    mv(g.btnAdd, x, by, 120, 28); x += 128;
    mv(g.btnLeft, x, by, 40, 28); x += 48;
    mv(g.btnRight, x, by, 40, 28); x += 48;
    mv(g.btnDelT, x, by, 90, 28);
}

static void onCommand(int id, int code) {
    if (id == IDC_SERIES && code == LBN_SELCHANGE) {
        int i = (int)SendMessageW(g.lbSeries, LB_GETCURSEL, 0, 0);
        if (i >= 0) selectSeries((int)SendMessageW(g.lbSeries, LB_GETITEMDATA, i, 0));
    } else if (id >= IDC_COL0 && id < IDC_COL0 + 3 && code == LBN_SELCHANGE) {
        int c = id - IDC_COL0;
        int i = (int)SendMessageW(g.lbCol[c], LB_GETCURSEL, 0, 0);
        if (i >= 0) {
            g.selStream = (int)SendMessageW(g.lbCol[c], LB_GETITEMDATA, i, 0);
            for (int k = 0; k < 3; ++k)
                if (k != c) SendMessageW(g.lbCol[k], LB_SETCURSEL, (WPARAM)-1, 0);
            updatePreview();
        }
    } else if ((id == IDC_EDNAME || id == IDC_EDTITLE || id == IDC_EDNOTIF) && code == EN_CHANGE && !g.loading) {
        updatePreview();
    } else if (code == BN_CLICKED) {
        switch (id) {
            case IDC_NEWS: actNewSeries(); break;
            case IDC_TOGGLE: actToggleActive(); break;
            case IDC_DELS: actDeleteSeries(); break;
            case IDC_SAVE: actSaveSeries(); break;
            case IDC_COPYT: copyToClipboard(GetText(g.stTitlePrev)); break;
            case IDC_COPYN: copyToClipboard(GetText(g.stNotifPrev)); break;
            case IDC_ADD: actAddStream(); break;
            case IDC_LEFT: actMoveStream(-1); break;
            case IDC_RIGHT: actMoveStream(+1); break;
            case IDC_DELT: actDeleteStream(); break;
        }
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            g.hwnd = hwnd;
            createControls();
            g.store.load(g.dataPath);
            g.curSeries = g.store.series.empty() ? 0 : g.store.series.front().id;
            refreshSeriesList();
            loadFields();
            return 0;
        }
        case WM_SIZE:
            if (wp != SIZE_MINIMIZED) layout(LOWORD(lp), HIWORD(lp));
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mmi = (MINMAXINFO*)lp;
            mmi->ptMinTrackSize.x = 960;
            mmi->ptMinTrackSize.y = 680;
            return 0;
        }
        case WM_COMMAND:
            onCommand(LOWORD(wp), HIWORD(wp));
            return 0;
        case WM_CTLCOLORSTATIC: {
            HWND h = (HWND)lp;
            if ((h == g.stTitleCount && g.titleOver) || (h == g.stNotifCount && g.notifOver)) {
                HDC dc = (HDC)wp;
                SetTextColor(dc, RGB(200, 0, 0));
                SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
                return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
            }
            break;
        }
        case WM_DESTROY:
            if (g.font) DeleteObject(g.font);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nShow) {
    // Datos en %APPDATA%\StreamPlanner\data.txt
    wchar_t buf[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH);
    std::filesystem::path dir = (n > 0 && n < MAX_PATH) ? std::filesystem::path(buf) : std::filesystem::path(L".");
    dir /= L"StreamPlanner";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    g.dataPath = dir / L"data.txt";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"StreamPlannerWnd";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Stream Planner", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1180, 720, nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return 1;
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {  // permite Tab entre controles
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}
