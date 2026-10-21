# Distribución v1 — Stream Planner

## Criterios de salida

- [ ] Configurar y compilar `x64 Release` desde una carpeta limpia.
- [ ] Verificar que SQLite se descargue desde el dominio oficial y se compile dentro de la aplicación.
- [ ] Probar creación, edición, activación y eliminación de series.
- [ ] Probar streams en los tres estados y etiquetas personalizadas, incluidos caracteres Unicode.
- [ ] Cerrar y volver a abrir la app; confirmar que los datos persisten.
- [ ] Probar migración desde una copia de `data.txt`; comparar cantidades y contenido antes/después.
- [ ] Confirmar que `data.txt` y `data.txt.migrated.bak` se conservan.
- [ ] Instalar en una cuenta de Windows de prueba, sin Visual Studio ni SQLite instalados.
- [ ] Actualizar sobre una instalación previa y comprobar que conserva la base de datos.
- [ ] Desinstalar y comprobar que `%LOCALAPPDATA%\StreamPlanner\stream_planner.db` sigue existiendo.
- [ ] Probar el instalador en Windows x64 limpio antes de publicar.

## Ubicaciones

- Ejecutable instalado: `%ProgramFiles%\Stream Planner\StreamPlanner.exe`.
- Base de datos: `%LOCALAPPDATA%\StreamPlanner\stream_planner.db`.
- Formato antiguo migrable: `%APPDATA%\StreamPlanner\data.txt`.
- Instalador compilado: `dist\StreamPlanner-Setup-1.0.0-x64.exe`.

## Política de datos

La base de datos es información del usuario. Nunca se incluye en el repositorio, instalador o paquetes de actualización. La desinstalación no debe borrarla. Mantener copias de seguridad antes de migraciones y cambios de esquema.
