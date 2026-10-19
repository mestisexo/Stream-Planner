#include "core/model.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace sp {

std::size_t utf8Length(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) ++n;
    return n;
}

std::string render(const std::string& tpl, const Series& s, const Stream& t) {
    std::string out;
    for (std::size_t i = 0; i < tpl.size(); ++i) {
        if (tpl[i] == '{') {
            std::size_t end = tpl.find('}', i);
            if (end != std::string::npos) {
                std::string key = tpl.substr(i + 1, end - i - 1);
                const std::string* val = nullptr;
                std::string num;
                if (key == "series") val = &s.name;
                else if (key == "topic") val = &t.topic;
                else if (key == "date") val = &t.date;
                else if (key == "day") { num = std::to_string(t.number); val = &num; }
                if (val) { out += *val; i = end; continue; }
            }
        }
        out += tpl[i];
    }
    return out;
}

// ---------- Persistencia: archivo de texto con campos separados por TAB ----------

static std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '\\') o += "\\\\";
        else if (c == '\t') o += "\\t";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') {}
        else o += c;
    }
    return o;
}

static std::string unesc(const std::string& s) {
    std::string o;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            o += (n == 't') ? '\t' : (n == 'n') ? '\n' : n;
        } else o += s[i];
    }
    return o;
}

static std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> f;
    std::string cur;
    for (char c : line) {
        if (c == '\t') { f.push_back(cur); cur.clear(); }
        else cur += c;
    }
    f.push_back(cur);
    return f;
}

bool Store::save(const std::filesystem::path& file) const {
    std::filesystem::path tmp = file;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        for (const auto& s : series)
            out << "S\t" << s.id << '\t' << esc(s.name) << '\t' << esc(s.titleTemplate) << '\t'
                << esc(s.notifTemplate) << '\t' << (s.active ? 1 : 0) << '\n';
        for (const auto& t : streams)
            out << "T\t" << t.id << '\t' << t.seriesId << '\t' << t.number << '\t' << esc(t.topic)
                << '\t' << esc(t.date) << '\t' << static_cast<int>(t.status) << '\n';
        if (!out) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, file, ec);  // reemplaza el anterior
    return !ec;
}

bool Store::load(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    series.clear();
    streams.clear();
    int maxId = 0;
    std::string line;
    while (std::getline(in, line)) {
        auto f = split(line);
        try {
            if (f[0] == "S" && f.size() >= 6) {
                Series s;
                s.id = std::stoi(f[1]);
                s.name = unesc(f[2]);
                s.titleTemplate = unesc(f[3]);
                s.notifTemplate = unesc(f[4]);
                s.active = f[5] == "1";
                maxId = std::max(maxId, s.id);
                series.push_back(s);
            } else if (f[0] == "T" && f.size() >= 7) {
                Stream t;
                t.id = std::stoi(f[1]);
                t.seriesId = std::stoi(f[2]);
                t.number = std::stoi(f[3]);
                t.topic = unesc(f[4]);
                t.date = unesc(f[5]);
                t.status = static_cast<Status>(std::clamp(std::stoi(f[6]), 0, 2));
                maxId = std::max(maxId, t.id);
                streams.push_back(t);
            }
        } catch (...) { /* línea corrupta: se ignora */ }
    }
    nextId_ = maxId + 1;
    return true;
}

// ---------- Operaciones ----------

int Store::addSeries(const std::string& name) {
    Series s;
    s.id = nextId_++;
    s.name = name;
    s.titleTemplate = "[{series}] Día {day} | {topic}";
    s.notifTemplate = "Hoy en directo: {series} día {day} - {topic}";
    series.push_back(s);
    return s.id;
}

int Store::addStream(int seriesId, const std::string& topic, const std::string& date) {
    Stream t;
    t.id = nextId_++;
    t.seriesId = seriesId;
    t.number = nextNumber(seriesId);
    t.topic = topic;
    t.date = date;
    streams.push_back(t);
    return t.id;
}

void Store::removeSeries(int id) {
    series.erase(std::remove_if(series.begin(), series.end(), [&](const Series& s) { return s.id == id; }), series.end());
    streams.erase(std::remove_if(streams.begin(), streams.end(), [&](const Stream& t) { return t.seriesId == id; }), streams.end());
}

void Store::removeStream(int id) {
    streams.erase(std::remove_if(streams.begin(), streams.end(), [&](const Stream& t) { return t.id == id; }), streams.end());
}

Series* Store::findSeries(int id) {
    for (auto& s : series) if (s.id == id) return &s;
    return nullptr;
}

Stream* Store::findStream(int id) {
    for (auto& t : streams) if (t.id == id) return &t;
    return nullptr;
}

int Store::count(int seriesId, Status st) const {
    int n = 0;
    for (const auto& t : streams) if (t.seriesId == seriesId && t.status == st) ++n;
    return n;
}

int Store::nextNumber(int seriesId) const {
    int m = 0;
    for (const auto& t : streams) if (t.seriesId == seriesId) m = std::max(m, t.number);
    return m + 1;
}

int Store::lastRealized(int seriesId) const {
    int m = 0;
    for (const auto& t : streams)
        if (t.seriesId == seriesId && t.status != Status::Planned) m = std::max(m, t.number);
    return m;
}

}  // namespace sp
