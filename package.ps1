$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$dist = Join-Path $root 'dist'
$scr = Join-Path $dist 'CatppuccinSaver.scr'
$installer = Join-Path $dist 'CatppuccinSaver-Setup.exe'
$vcVars = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsx86_amd64.bat'
$source = Join-Path $root 'CatppuccinSaver.cpp'
$csc = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$manifest = Join-Path $root 'setup.manifest'
$setupSource = Join-Path $root 'Setup.cs'

if (-not (Test-Path $csc)) { throw "C# compiler was not found at $csc" }
if (-not (Test-Path $vcVars)) { throw "Visual Studio build environment not found at $vcVars" }
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
$command = "call `"$vcVars`" && cl /nologo /std:c++17 /O2 /EHsc /DUNICODE /D_UNICODE `"$source`" /Fe:`"$scr`" /link /SUBSYSTEM:WINDOWS"
cmd.exe /d /s /c $command
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $scr)) { throw 'Screensaver compilation failed.' }
& $csc /nologo /target:winexe /optimize+ /out:$installer /win32manifest:$manifest /resource:$scr,CatppuccinSaver.scr $setupSource
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $installer)) { throw 'Setup.exe packaging failed.' }
Write-Host "Created $installer"
