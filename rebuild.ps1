$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& (Join-Path $root 'package.ps1')
Write-Host 'Rebuilt both CatppuccinSaver.scr and CatppuccinSaver-Setup.exe.'
