#!/usr/bin/env bash
# Makes the Linux AppImage from the builds of packaging/build_linux.sh. No game data goes in: players choose their own
# disc image in the launcher.
#
#   APPIMAGETOOL=/path/to/appimagetool packaging/build_appimage.sh
#
# Writes dist/linux/MuppetRaceManiaRecompiled-v<VERSION>-linux-x86_64.AppImage.
#
# A build keeps its settings and files beside itself and an AppImage cannot be written to, so AppRun copies the builds
# to <user data>/muppet_racemania/games/muppet_racemania, where the launcher looks for them (TRG Launcher 2.0,
# GameExePath). A new AppImage replaces only the release's files there; the disc image, settings and saves stay.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(tr -d '\r\n' < "$ROOT/VERSION")"
TOOL="${APPIMAGETOOL:-$HOME/appimagetool-x86_64.AppImage}"
OUT="${OUT_DIR:-$ROOT/dist/linux}"
TRG="${TRG_LAUNCHER_DIR:-$ROOT/third_party/trg-launcher}"
LAUNCHER="$ROOT/build-linux-launcher/MuppetRaceMania"

[ -f "$LAUNCHER" ] || { echo "$LAUNCHER is missing: run packaging/build_linux.sh" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
APPDIR="$WORK/AppDir"
GAMES="$APPDIR/games"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" "$GAMES"
cp "$LAUNCHER" "$APPDIR/usr/bin/"

# One build per disc: the same files as the Windows release (packaging/make_release.ps1).
for disc in usa eur; do
  bin="$ROOT/build-linux-$disc"
  exe="$bin/Muppet_RaceMania_Recompiled"
  [ -f "$exe" ] || { echo "$exe is missing: run packaging/build_linux.sh" >&2; exit 1; }
  built="$(tr -d '\r\n' < "$bin/psx_game_version.txt")"
  [ "$built" = "$VERSION" ] || { echo "build-linux-$disc is version $built, VERSION says $VERSION: rebuild it" >&2; exit 1; }
  dest="$GAMES/$disc"
  mkdir -p "$dest"
  cp "$exe" "$bin/game.toml" "$bin/game_options.toml" "$bin/psx_game_version.txt" "$dest/"
  cp -r "$bin/assets" "$bin/bios" "$bin/mods" "$dest/"
  rm -f "$dest/mods/state.toml"  # the player's, not the release's
  strip --strip-unneeded "$dest/Muppet_RaceMania_Recompiled"
done
strip --strip-unneeded "$APPDIR/usr/bin/MuppetRaceMania"

# The C++ runtime for older distributions (AppRun decides), and the launcher's font (Noto Sans, OFL).
for lib in libstdc++.so.6 libgcc_s.so.1; do cp -L "/usr/lib/x86_64-linux-gnu/$lib" "$APPDIR/usr/lib/"; done
mkdir -p "$APPDIR/usr/share/fonts" "$APPDIR/usr/share/licenses/noto-sans"
cp "$TRG/launcher2/linux/fonts/"NotoSans-*.ttf "$APPDIR/usr/share/fonts/"
cp "$TRG/launcher2/linux/fonts/OFL.txt" "$APPDIR/usr/share/licenses/noto-sans/"

# The licences of what the release is built from, as they ask.
LICENSES="$APPDIR/usr/share/licenses/muppet_racemania"
mkdir -p "$LICENSES"
cp "$ROOT/psxrecomp/LICENSE" "$LICENSES/psxrecomp-LICENSE.txt"
cp "$ROOT/psxrecomp/THIRD_PARTY_ATTRIBUTION.md" "$LICENSES/psxrecomp-THIRD_PARTY_ATTRIBUTION.md"
cp "$ROOT/psxrecomp/runtime/licenses/"* "$LICENSES/"
cp "$ROOT/psxrecomp/runtime/third_party/fsr1/LICENSE.txt" "$LICENSES/FSR1-LICENSE.txt"
cp "$ROOT/psxrecomp/runtime/third_party/nis/LICENSE.txt" "$LICENSES/NIS-LICENSE.txt"
cp "$TRG/LICENSE" "$LICENSES/trg-launcher-LICENSE.txt"

cp "$ROOT/docs/steam/icon.png" "$APPDIR/muppet_racemania.png"
cat > "$APPDIR/muppet_racemania.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Muppet RaceMania Recompiled
Comment=Muppet RaceMania, a native PC port of the PlayStation game
Exec=MuppetRaceMania
Icon=muppet_racemania
Categories=Game;
Terminal=false
DESKTOP

cat > "$APPDIR/AppRun" <<'APPRUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
DATA="${XDG_DATA_HOME:-$HOME/.local/share}/muppet_racemania"
GAME="$DATA/games/muppet_racemania"
# Each AppImage file (its size and time) has its own copy of the program.
ID="$(stat -c '%s-%Y' "$APPIMAGE" 2>/dev/null)"
[ -n "$ID" ] || ID=extracted

# The builds, where the launcher starts them from. Everything else in that folder is the player's and stays.
if [ ! -f "$GAME/.appimage-$ID" ]; then
  mkdir -p "$GAME"
  rm -f "$GAME"/.appimage-*
  cp -rf "$HERE/games/." "$GAME/" && touch "$GAME/.appimage-$ID"
fi

# The launcher runs from a copy outside the AppImage's mount (as King Kong Recompiled does). The mount is served by a
# helper process that ends as soon as it gets a signal; Steam signals every process of a game at once when it stops
# it, so a game still quitting could lose its libraries and crash. MUPPET_APPIMAGE_COPY=0 runs from the mount.
if [ -n "$APPIMAGE" ] && [ "${MUPPET_APPIMAGE_COPY:-1}" != 0 ]; then
  BASE="${XDG_CACHE_HOME:-$HOME/.cache}/muppet_racemania/appimage"
  if [ ! -f "$BASE/$ID/.complete" ]; then
    for old in "$BASE"/*; do [ "$old" = "$BASE/$ID" ] || rm -rf "$old"; done
    PART="$BASE/.part-$$"
    rm -rf "$BASE/$ID"
    if mkdir -p "$PART" && cp -a "$HERE/usr" "$PART/" && touch "$PART/.complete"; then
      mv -T "$PART" "$BASE/$ID" 2>/dev/null || rm -rf "$PART"
    else
      rm -rf "$PART"
    fi
  fi
  [ ! -f "$BASE/$ID/.complete" ] || HERE="$BASE/$ID"
fi

# The bundled C++ runtime only when the system's is older than the one the program needs; a newer system copy is kept
# for the graphics drivers.
SYS_CXX="$(ldconfig -p 2>/dev/null | awk '/libstdc\+\+\.so\.6 .*x86-64/ {print $NF; exit}')"
if [ -z "$SYS_CXX" ] || ! grep -q GLIBCXX_3.4.32 "$SYS_CXX" 2>/dev/null; then
  export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
exec "$HERE/usr/bin/MuppetRaceMania" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# Nothing from the game may ship: the player's own disc provides it.
if find "$APPDIR" \( -iname '*.cue' -o -iname '*.iso' -o -iname '*.img' -o -iname '*.str' -o -iname '*.xa' -o -iname 'SLUS_*' -o -iname 'SCES_*' \) | grep .; then
  echo "error: files from the game are in the AppDir (above)" >&2
  exit 1
fi

for f in "$APPDIR/usr/bin/MuppetRaceMania" "$GAMES/usa/Muppet_RaceMania_Recompiled" "$GAMES/eur/Muppet_RaceMania_Recompiled"; do
  if ldd "$f" | grep 'not found'; then echo "error: $f needs a library that is not there" >&2; exit 1; fi
done

mkdir -p "$OUT"
FILE="$OUT/MuppetRaceManiaRecompiled-v$VERSION-linux-x86_64.AppImage"
rm -f "$FILE"
ARCH=x86_64 "$TOOL" --appimage-extract-and-run "$APPDIR" "$FILE" > "$WORK/appimagetool.log" 2>&1 || { tail -20 "$WORK/appimagetool.log"; exit 1; }
ls -la "$FILE"
sha256sum "$FILE"
