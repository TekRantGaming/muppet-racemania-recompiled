# Building Muppet RaceMania Recompiled

| Path | What |
|---|---|
| `muppet_mods.c`, `mods/preloaded/packages/muppet.pc` | The port's own code and its options: widescreen, frame rate, draw distance |
| `game.toml`, `game_eur.toml`, `seeds/` | What the recompiler needs for the USA and Europe discs |
| `launcher/` | The launcher: [TRG Launcher 2.0](https://github.com/TekRantGaming/trg-launcher) with one project file (`project.cpp`) |
| `psxrecomp/`, `third_party/trg-launcher/` | The recompiler and runtime, and the launcher library (submodules) |
| `packaging/` | The Windows release script and the Linux build and AppImage scripts |

```
git clone --recursive https://github.com/TekRantGaming/muppet-racemania-recompiled.git
```

## Requirements

* **Windows:** Visual Studio 2022 Build Tools with the C++ tools and its LLVM clang, CMake, Ninja, Python 3.
* **Linux:** clang and lld 20, CMake, Ninja, Python 3, and the development packages for X11, Wayland, OpenGL, ALSA and
  PulseAudio. The releases are built on Ubuntu 24.04.
* Your own disc image for each region you build. Both regions need a separate build, because they are different
  programs.

## The game code

The game's code is generated from your own disc. It stays in `generated/` on your PC and is never committed:

```
python psxrecomp\psxrecomp_cli.py generate --config game.toml --project-root . --disc "path\to\Muppet RaceMania (USA).cue"
python psxrecomp\psxrecomp_cli.py generate --config game_eur.toml --project-root . --disc "path\to\Muppet RaceMania (Europe).cue"
```

Run both before a release: each writes its own files, named after the disc's program.

## Windows

```
build.bat USA
build.bat EUR
build-launcher.bat
```

Output: `build-usa\`, `build-eur\` and `build-launcher\MuppetRaceMania.exe`. `build-dev.bat` makes a build with the
debug tools.

## Linux

```
packaging/build_linux.sh
APPIMAGETOOL=/path/to/appimagetool packaging/build_appimage.sh
```

Output: `dist/linux/MuppetRaceManiaRecompiled-v<VERSION>-linux-x86_64.AppImage`. In WSL, build on Linux's own disk
rather than under `/mnt`.

## Releases

Set `VERSION` to the release's version, build everything above, then on Windows:

```
powershell -ExecutionPolicy Bypass -File packaging\make_release.ps1 -Version 1.0.0 -Out dist\releases
```

It writes `dist\releases\v1.0.0\`: the zip players download, and in `packages\` what the launcher downloads, with
`SHA256SUMS.txt`. Each version has two GitHub releases, as the
[TRG Launcher](https://github.com/TekRantGaming/trg-launcher/blob/main/launcher2/README.md#releases-and-updates)
describes. Create `packages-v1.0.0` first, then `v1.0.0`, so the players' release stays the latest. The AppImage goes in
both.
