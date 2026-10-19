// Núcleo portable: NO incluir nada de windows.h aquí.
#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace sp {

// Límite de Twitch para título del stream y para la notificación de "go live".
// (Verifica el valor vigente en la documentación de Twitch si algún día cambia.)
constexpr std::size_t kTwitchLimit = 140;

enum class Status { Planned = 0, Live = 1, Done = 2 };  // columnas del kanban

struct Series {
    int id = 0;
    std::string name;
    std::string titleTemplate;  // p.ej. "[{series}] Día {day} | {topic}"
    std::string notifTemplate;  // p.ej. "Hoy en directo: {series} día {day} - {topic}"
    bool active = true;
};

struct Stream {
    int id = 0;
    int seriesId = 0;
    int number = 0;  // "día N" dentro de la serie
    std::string topic;
    std::string date;  // YYYY-MM-DD
    Status status = Status::Planned;
};

// Cuenta caracteres (code points) de un string UTF-8.
std::size_t utf8Length(const std::string& s);

// Reemplaza {series} {day} {topic} {date} en la plantilla.
std::string render(const std::string& tpl, const Series& s, const Stream& t);

class Store {
public:
    std::vector<Series> series;
    std::vector<Stream> streams;

    bool load(const std::filesystem::path& file);
    bool save(const std::filesystem::path& file) const;

    int addSeries(const std::string& name);
    int addStream(int seriesId, const std::string& topic, const std::string& date);
    void removeSeries(int id);  // borra también sus streams
    void removeStream(int id);

    Series* findSeries(int id);
    Stream* findStream(int id);

    int count(int seriesId, Status st) const;
    int nextNumber(int seriesId) const;    // siguiente "día" a asignar
    int lastRealized(int seriesId) const;  // último día en vivo/hecho (0 si ninguno)

private:
    int nextId_ = 1;
};

}  // namespace sp
