// Almacén en memoria de series y streams + persistencia.
#pragma once
#include <filesystem>
#include <vector>

#include "core/types.h"

namespace sp {

class Store {
public:
    std::vector<Series> series;
    std::vector<Stream> streams;

    bool load(const std::filesystem::path& file, const std::filesystem::path& legacyFile = {}); // SQLite + migración
    bool save(const std::filesystem::path& file) const;  // persistence.cpp

    int addSeries(const std::string& name);
    int addStream(int seriesId, int day, const std::string& date, std::vector<TagValue> tags);
    void removeSeries(int id);  // borra también sus streams
    void removeStream(int id);

    Series* findSeries(int id);
    Stream* findStream(int id);

    int count(int seriesId, Status st) const;
    int latestDay(int seriesId) const;  // mayor día usado en la serie (1 si no hay streams)

private:
    int nextId_ = 1;
};

}  // namespace sp
