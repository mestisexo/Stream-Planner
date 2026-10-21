#include "core/text.h"

#include <algorithm>
#include <charconv>

namespace sp {

namespace {

// Busca el '}' que cierra la '{' en la posición i y valida que lo de dentro sea un nombre de etiqueta.
bool matchKey(std::string_view tpl, std::size_t i, std::string_view& key, std::size_t& end) {
    end = tpl.find('}', i);
    if (end == std::string_view::npos) return false;
    key = tpl.substr(i + 1, end - i - 1);
    return isValidTagName(key);
}

}  // namespace

std::size_t utf8Length(std::string_view s) {
    std::size_t n = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) ++n;
    return n;
}

bool isReservedTag(std::string_view key) {
    return key == "series" || key == "day" || key == "date";
}

bool isValidTagName(std::string_view name) {
    if (name.empty() || name.size() > kMaxTagNameLen) return false;
    for (unsigned char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                        c == '_' || c >= 0x80;
        if (!ok) return false;
    }
    return true;
}

std::vector<std::string> extractTags(std::string_view titleTpl, std::string_view notifTpl) {
    std::vector<std::string> out;
    for (std::string_view tpl : {titleTpl, notifTpl}) {
        for (std::size_t i = 0; i < tpl.size(); ++i) {
            if (tpl[i] != '{') continue;
            std::string_view key;
            std::size_t end;
            if (!matchKey(tpl, i, key, end)) continue;
            if (!isReservedTag(key) &&
                std::none_of(out.begin(), out.end(), [&](const std::string& s) { return std::string_view(s) == key; }))
                out.emplace_back(key);
            i = end;
        }
    }
    return out;
}

std::string render(std::string_view tpl, const Series& s, const Stream& t) {
    std::size_t extra = s.name.size() + t.date.size();
    for (const auto& tv : t.tags) extra += tv.value.size();
    std::string out;
    out.reserve(tpl.size() + extra);
    for (std::size_t i = 0; i < tpl.size(); ++i) {
        if (tpl[i] == '{') {
            std::string_view key;
            std::size_t end;
            if (matchKey(tpl, i, key, end)) {
                if (key == "series") out += s.name;
                else if (key == "date") out += t.date;
                else if (key == "day") {
                    char b[16];
                    auto r = std::to_chars(b, b + sizeof b, t.number);
                    out.append(b, r.ptr);
                } else if (const std::string* v = t.findTag(key)) out += *v;
                i = end;
                continue;
            }
        }
        out += tpl[i];
    }
    return out;
}

}  // namespace sp
