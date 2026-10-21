// Refrescos de UI: leen el modelo y actualizan los controles.
#include <algorithm>
#include <vector>

#include "platform/win32/app.h"

void refreshSeriesList() {
    SendMessageW(g.lbSeries, LB_RESETCONTENT, 0, 0);
    int sel = -1;
    std::wstring t;  // un solo buffer reutilizado
    for (auto& s : g.store.series) {
        t = s.active ? L"[activa] " : L"[finalizada] ";
        appendW(t, s.name);
        int i = (int)SendMessageW(g.lbSeries, LB_ADDSTRING, 0, (LPARAM)t.c_str());
        SendMessageW(g.lbSeries, LB_SETITEMDATA, i, s.id);
        if (s.id == g.curSeries) sel = i;
    }
    SendMessageW(g.lbSeries, LB_SETCURSEL, sel, 0);
}

void refreshKanban() {
    for (int c = 0; c < 3; ++c) SendMessageW(g.lbCol[c], LB_RESETCONTENT, 0, 0);
    sp::Series* s = cur();
    if (!s) return;
    std::vector<const sp::Stream*> v;
    v.reserve(g.store.streams.size());
    for (auto& t : g.store.streams)
        if (t.seriesId == s->id) v.push_back(&t);
    // Por día; a igual día, por orden de creación (varios streams pueden compartir día)
    std::sort(v.begin(), v.end(), [](const sp::Stream* a, const sp::Stream* b) {
        return a->number != b->number ? a->number < b->number : a->id < b->id;
    });
    std::wstring txt;  // un solo buffer reutilizado
    for (const sp::Stream* t : v) {
        int c = (int)t->status;
        txt = L"Día ";
        txt += std::to_wstring(t->number);
        bool first = true;
        for (const auto& tv : t->tags) {
            if (tv.value.empty()) continue;
            txt += first ? L" - " : L" · ";
            first = false;
            appendW(txt, tv.value);
        }
        txt += L"  (";
        appendW(txt, t->date);
        txt += L')';
        int i = (int)SendMessageW(g.lbCol[c], LB_ADDSTRING, 0, (LPARAM)txt.c_str());
        SendMessageW(g.lbCol[c], LB_SETITEMDATA, i, t->id);
        if (t->id == g.selStream) SendMessageW(g.lbCol[c], LB_SETCURSEL, i, 0);
    }
}

void updateStats() {
    sp::Series* s = cur();
    if (!s) { SetWindowTextW(g.stStats, L"Crea o selecciona una serie."); return; }
    std::wstring t = L"Planeado: " + std::to_wstring(g.store.count(s->id, sp::Status::Planned)) +
        L"   |   En vivo: " + std::to_wstring(g.store.count(s->id, sp::Status::Live)) +
        L"   |   Realizado: " + std::to_wstring(g.store.count(s->id, sp::Status::Done));
    SetWindowTextW(g.stStats, t.c_str());
}

int readDay() {
    std::wstring t = GetText(g.edDay);
    if (t.empty() || t.size() > 5) return 0;
    int v = 0;
    for (wchar_t c : t) {
        if (c < L'0' || c > L'9') return 0;
        v = v * 10 + (c - L'0');
    }
    return v;
}

void updatePreview() {
    // render() solo usa el nombre de la serie: no hace falta copiar las plantillas a una Series.
    sp::Series tmp;
    tmp.name = U8(GetText(g.edName));
    const std::string titleTpl = U8(GetText(g.edTitle));
    const std::string notifTpl = U8(GetText(g.edNotif));

    sp::Stream sample;
    const sp::Stream* st = g.store.findStream(g.selStream);
    if (!st) {
        // Sin stream seleccionado: vista previa con lo que hay en el formulario
        int day = readDay();
        sample.number = day > 0 ? day : g.store.latestDay(g.curSeries);
        std::string d = U8(GetText(g.edDate));
        sample.date = d.empty() ? today() : d;
        for (size_t i = 0; i < g.tags.size() && i < sp::kMaxTags; ++i) {
            std::string v = U8(GetText(g.tagEd[i]));
            sample.setTag(g.tags[i], v.empty() ? "<" + g.tags[i] + ">" : v);
        }
        st = &sample;
    }
    std::string t = sp::render(titleTpl, tmp, *st);
    std::string n = sp::render(notifTpl, tmp, *st);
    size_t lt = sp::utf8Length(t), ln = sp::utf8Length(n);
    g.titleOver = lt > sp::kTwitchLimit;
    g.notifOver = ln > sp::kTwitchLimit;

    SetWindowTextW(g.stTitlePrev, W(t).c_str());
    SetWindowTextW(g.stNotifPrev, W(n).c_str());
    std::wstring ct = L"Título: " + std::to_wstring(lt) + L"/" + std::to_wstring(sp::kTwitchLimit) + (g.titleOver ? L"  ¡DEMASIADO LARGO!" : L"");
    std::wstring cn = L"Notificación: " + std::to_wstring(ln) + L"/" + std::to_wstring(sp::kTwitchLimit) + (g.notifOver ? L"  ¡DEMASIADA LARGA!" : L"");
    SetWindowTextW(g.stTitleCount, ct.c_str());
    SetWindowTextW(g.stNotifCount, cn.c_str());
    InvalidateRect(g.stTitleCount, nullptr, TRUE);
    InvalidateRect(g.stNotifCount, nullptr, TRUE);
}

// Detecta las {etiquetas} de las plantillas (lo que hay escrito ahora mismo en los campos)
// y muestra/oculta los campos de valor. Lo ya escrito en un campo se conserva por nombre.
void syncTagFields(bool force) {
    std::vector<std::string> now = sp::extractTags(U8(GetText(g.edTitle)), U8(GetText(g.edNotif)));
    const size_t total = now.size();
    g.tagsOver = total > sp::kMaxTags;
    if (g.tagsOver) now.resize(sp::kMaxTags);  // la UI solo maneja las primeras 10

    // Línea informativa de la sección "Etiquetas"
    std::wstring info = L"(" + std::to_wstring(total) + L"/" + std::to_wstring(sp::kMaxTags) + L")";
    if (total == 0) info += L"  ninguna todavía";
    for (const std::string& name : now) {
        info += L"  {";
        appendW(info, name);
        info += L'}';
    }
    if (g.tagsOver) info += L"  ¡DEMASIADAS!";
    if (info != g.tagsInfo) {
        g.tagsInfo = info;
        SetWindowTextW(g.stTags, info.c_str());
        InvalidateRect(g.stTags, nullptr, TRUE);
    }

    if (!force && now == g.tags) return;

    std::vector<std::wstring> oldVals;
    oldVals.reserve(g.tags.size());
    for (size_t i = 0; i < g.tags.size(); ++i) oldVals.push_back(GetText(g.tagEd[i]));
    const std::vector<std::string> oldNames = std::move(g.tags);
    g.tags = std::move(now);

    const bool prev = g.loading;
    g.loading = true;  // evita que los SetWindowText disparen updatePreview
    for (size_t i = 0; i < sp::kMaxTags; ++i) {
        const bool on = i < g.tags.size();
        if (on) {
            std::wstring val;
            for (size_t j = 0; j < oldNames.size(); ++j)
                if (oldNames[j] == g.tags[i]) { val = oldVals[j]; break; }
            SetWindowTextW(g.tagLbl[i], W(g.tags[i]).c_str());
            SetWindowTextW(g.tagEd[i], val.c_str());
        }
        ShowWindow(g.tagLbl[i], on ? SW_SHOW : SW_HIDE);
        ShowWindow(g.tagEd[i], on ? SW_SHOW : SW_HIDE);
    }
    g.loading = prev;
    relayout();
}

void loadFields() {
    g.loading = true;
    sp::Series* s = cur();
    SetWindowTextW(g.edName, s ? W(s->name).c_str() : L"");
    SetWindowTextW(g.edTitle, s ? W(s->titleTemplate).c_str() : L"");
    SetWindowTextW(g.edNotif, s ? W(s->notifTemplate).c_str() : L"");
    SetWindowTextW(g.edDay, s ? std::to_wstring(g.store.latestDay(s->id)).c_str() : L"");
    for (size_t i = 0; i < sp::kMaxTags; ++i) SetWindowTextW(g.tagEd[i], L"");  // valores nuevos en blanco
    g.tags.clear();
    g.loading = false;
    syncTagFields(true);
    refreshKanban();
    updatePreview();
    updateStats();
}

void selectSeries(int id) {
    g.curSeries = id;
    g.selStream = 0;
    refreshSeriesList();
    loadFields();
}
