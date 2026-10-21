// Utilidades de texto: longitud UTF-8, etiquetas y render de plantillas.
#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace sp {

// Cuenta caracteres (code points) de un string UTF-8.
std::size_t utf8Length(std::string_view s);

// Etiquetas con origen propio: {series}, {day}, {date}. Las demás son personalizadas.
bool isReservedTag(std::string_view key);

// Nombre válido: 1..kMaxTagNameLen, solo letras/números/_ (incluye letras UTF-8), sin espacios.
bool isValidTagName(std::string_view name);

// Etiquetas personalizadas usadas en las plantillas, en orden de aparición y sin repetir.
std::vector<std::string> extractTags(std::string_view titleTpl, std::string_view notifTpl);

// Reemplaza {series} {day} {date} y cualquier etiqueta personalizada {nombre}.
// Una etiqueta sin valor en el stream se reemplaza por texto vacío.
std::string render(std::string_view tpl, const Series& s, const Stream& t);

}  // namespace sp
