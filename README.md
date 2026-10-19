# Stream Planner

## Estructura
- `src/core/`            Lógica portable (C++ estándar): series, streams, plantillas, límite de Twitch, guardado.
- `src/platform/win32/`  Interfaz Win32. Para Linux se agregará `src/platform/linux/` (GTK o Qt) reutilizando `core/`.

## Compilar en Visual Studio
1. Archivo > Abrir > Carpeta... y elige esta carpeta (VS detecta `CMakeLists.txt`).
2. Espera a que diga "La generación de CMake finalizó".
3. Arriba, en el selector de inicio elige `StreamPlanner.exe`, y F5 (o Ctrl+F5).

Datos guardados en: `%APPDATA%\StreamPlanner\data.txt`
