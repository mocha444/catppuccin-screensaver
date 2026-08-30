# Catppuccin Screensaver

A native Windows screensaver inspired by Catppuccin Mocha. It includes smooth high-FPS star animation, white sparkle effects, a 12-hour clock, live weather, wind speed, and a seven-day forecast.

## Build

Open PowerShell as Administrator, change to this folder, and run:

```powershell
.\rebuild-and-install.ps1
```

This creates both files in `dist`:

- `CatppuccinSaver.scr`: the screensaver executable
- `CatppuccinSaver-Setup.exe`: the administrator-elevated installer

## Install

Run `CatppuccinSaver-Setup.exe`, approve the Windows elevation prompt, and select CatppuccinSaver in Screen Saver Settings.

For the latest ready-to-install executable, download it from the [GitHub Releases page](https://github.com/blobster444/catppuccin-screensaver/releases).

Windows Location must be enabled for location-based weather. If Windows cannot provide coordinates, the saver falls back to network-based location.

## Uninstall

Run PowerShell as Administrator and execute:

```powershell
.\uninstall.ps1
```

## Development

After changing `CatppuccinSaver.cpp`, run `rebuild-and-install.ps1` as Administrator to regenerate both distributable files and launch the fresh installer. The project uses the Visual Studio MSVC build tools and Windows SDK.
