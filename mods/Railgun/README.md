# Railgun

Railgun adds a buildable, powered ship weapon with its own ammunition and operator
station to The Last Caretaker.

## Build

To build and install, close the game and run from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods\Railgun\Build-Railgun.ps1 -Install
```

Omit `-Install` to prepare a build under `Tmp/Railgun/` without installing it.
The candidate ZIP maps the container, adjacent INI, and content-plugin
descriptor to their game paths through Vortex override instructions.
