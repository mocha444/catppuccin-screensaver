# Catppuccin Screensaver

A native Windows screensaver inspired by Catppuccin Mocha. It includes smooth high-FPS star animation, white sparkle effects, a 12-hour clock, live weather, wind speed, and a seven-day forecast.

## Build

From this folder, run PowerShell:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\rebuild.ps1
```

This creates both files in `dist`:

- `CatppuccinSaver.scr`: the screensaver executable
- `CatppuccinSaver-Setup.exe`: the administrator-elevated installer

## Install

Run `CatppuccinSaver-Setup.exe`, approve the Windows elevation prompt, and select CatppuccinSaver in Screen Saver Settings.

Windows Location must be enabled for location-based weather. If Windows cannot provide coordinates, the saver falls back to network-based location.

## Uninstall

Run PowerShell as Administrator and execute:

```powershell
.\uninstall.ps1
```

## Development

After changing `CatppuccinSaver.cpp`, run `rebuild.ps1` to regenerate both distributable files. The project uses the Visual Studio MSVC build tools and Windows SDK.
