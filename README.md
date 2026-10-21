# Stream Planner

Aplicación de escritorio nativa para Windows x64, construida con C++17 y Win32 API. La persistencia usa SQLite integrado desde la amalgamación oficial.

## Datos del usuario

La base de datos se guarda en `%LOCALAPPDATA%\StreamPlanner\stream_planner.db`. No se guarda en `Program Files`, no se versiona en Git y no se elimina al desinstalar.

Si existe el formato anterior `%APPDATA%\StreamPlanner\data.txt`, la primera ejecución crea la base SQLite e importa las series, streams y etiquetas. El archivo original se conserva y se intenta crear una copia `data.txt.migrated.bak`.

## Requisitos para compilar

- Windows 10/11 x64.
- Visual Studio 2022 Build Tools o Visual Studio con **Desarrollo para el escritorio con C++** y Windows SDK.
- CMake 3.20 o posterior.
- Conexión a Internet la primera vez para descargar la amalgamación oficial de SQLite 3.54.1 desde `sqlite.org` durante la configuración de CMake.
- Inno Setup 6 para crear el instalador.

SQLite se compila estáticamente en la aplicación. El runtime de MSVC también se configura estáticamente para evitar que el usuario final tenga que instalar el Visual C++ Redistributable por separado.

## Compilar Release

Desde PowerShell, en la carpeta raíz del repositorio:

```powershell
cmake -S . -B out/build/x64-Release -A x64
cmake --build out/build/x64-Release --config Release
```

Ejecutable esperado: `out\build\x64-Release\StreamPlanner.exe`.

> La primera configuración descarga SQLite. Después, CMake reutiliza la fuente descargada. No compartas la carpeta de datos del usuario ni incluyas archivos `.db` en el repositorio.

## Crear el instalador

1. Compila Release con los comandos anteriores.
2. Instala Inno Setup 6.
3. Abre `installer\StreamPlanner.iss` con Inno Setup Compiler y pulsa **Compile**.
4. El instalador se genera en `dist\StreamPlanner-Setup-1.0.0-x64.exe`.

El script instala el ejecutable, crea accesos directos y no define ninguna acción para borrar los datos del usuario al desinstalar. Antes de publicar, prueba el instalador en un usuario de Windows limpio y valida una instalación encima de una versión anterior.

## Modelo de datos

- `series`: nombre, plantillas y estado activo.
- `streams`: serie, número/día, estado y fecha.
- `tag_values`: valores de etiquetas por stream.

Las operaciones de guardado se realizan en transacción. SQLite usa claves foráneas, índice por serie, WAL y sincronización completa.

## Copias de seguridad

Con la aplicación cerrada, copia `stream_planner.db` desde `%LOCALAPPDATA%\StreamPlanner\` a una ubicación segura. También se puede copiar con la aplicación abierta, pero para una copia consistente se recomienda usar la API de backup de SQLite o cerrar primero la aplicación. No borres el respaldo de `data.txt` hasta comprobar la migración.

## Estructura

- `src/core/`: dominio, plantillas, almacenamiento y persistencia.
- `src/platform/win32/`: interfaz Win32.
- `installer/`: script de Inno Setup.
- `docs/`: documentación del proyecto.
