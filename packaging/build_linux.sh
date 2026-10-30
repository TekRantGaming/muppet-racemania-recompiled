#!/usr/bin/env bash
# Builds Muppet RaceMania Recompiled for Linux: the game once per disc (build-linux-usa/, build-linux-eur/) and the
# launcher (build-linux-launcher/). BUILDING.md says what it needs. In WSL, build on Linux's own disk, not /mnt.
#
#   packaging/build_linux.sh [usa|eur|launcher ...]     (default: all three)
set -euo pipefail
cd "$(dirname "$0")/.."
export CC="${CC:-clang-20}" CXX="${CXX:-clang++-20}"
LLD="${LLD:-lld}"
jobs="${JOBS:-$(nproc)}"
parts=("$@")
[ ${#parts[@]} -gt 0 ] || parts=(usa eur launcher)

for part in "${parts[@]}"; do
  case "$part" in
    usa|eur)
      out="build-linux-$part"
      cmake -S . -B "$out" -G Ninja -DCMAKE_BUILD_TYPE=Release -DMRM_REGION="${part^^}" -DPSX_RUNTIME_IPO=ON \
        -DCMAKE_DISABLE_FIND_PACKAGE_SDL3=ON -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=$LLD"
      cmake --build "$out" --target psx-runtime -j "$jobs"
      ;;
    launcher)
      trg_arg=()
      [ -z "${TRG_LAUNCHER_DIR:-}" ] || trg_arg=("-DTRG_LAUNCHER_DIR=$TRG_LAUNCHER_DIR")
      cmake -S launcher -B build-linux-launcher -G Ninja -DCMAKE_BUILD_TYPE=Release ${trg_arg[@]+"${trg_arg[@]}"}
      cmake --build build-linux-launcher --target MuppetRaceMania -j "$jobs"
      ;;
    *) echo "usage: $0 [usa|eur|launcher ...]" >&2; exit 2 ;;
  esac
done
