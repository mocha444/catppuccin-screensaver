$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$installer = Join-Path $root 'dist\CatppuccinSaver-Setup.exe'
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& (Join-Path $root 'package.ps1')
Write-Host 'Rebuilt both CatppuccinSaver.scr and CatppuccinSaver-Setup.exe.'
if (-not (Test-Path $installer)) { throw "Installer was not created at $installer" }
Write-Host 'Launching the installer...'
Start-Process -FilePath $installer -Wait
