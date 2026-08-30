$ErrorActionPreference = 'Stop'
$target = Join-Path $env:WINDIR 'System32\CatppuccinSaver.scr'
if (Test-Path $target) {
    Remove-Item $target -Force
    Write-Host "Removed $target"
} else {
    Write-Host 'CatppuccinSaver is not installed.'
}
