$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

Write-Host 'Configurando Stream Planner x64 Release...'
cmake -S . -B out/build/x64-Release -A x64
if ($LASTEXITCODE -ne 0) { throw 'Falló la configuración de CMake.' }

Write-Host 'Compilando...'
cmake --build out/build/x64-Release --config Release
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación.' }

$exe = Join-Path $root 'out/build/x64-Release/StreamPlanner.exe'
if (!(Test-Path $exe)) { throw "No se encontró el ejecutable esperado: $exe" }
Write-Host "Release listo: $exe" -ForegroundColor Green
Write-Host 'Para crear el instalador, abre installer\StreamPlanner.iss con Inno Setup 6 y pulsa Compile.'
