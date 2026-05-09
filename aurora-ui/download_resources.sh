#!/bin/bash
CORES_DIR="src-tauri/resources/cores"
SYSTEM_DIR="src-tauri/resources/system"
BASE_URL="https://buildbot.libretro.com/nightly/apple/osx/arm64/latest"

echo "Downloading Cores..."
cores=(
  "fceumm_libretro.dylib.zip"
  "snes9x_libretro.dylib.zip"
  "genesis_plus_gx_libretro.dylib.zip"
  "pcsx_rearmed_libretro.dylib.zip"
  "mgba_libretro.dylib.zip"
  "gambatte_libretro.dylib.zip"
  "mupen64plus_next_libretro.dylib.zip"
)

for core in "${cores[@]}"; do
  echo "Fetching $core..."
  curl -L "$BASE_URL/$core" -o "$CORES_DIR/$core"
  unzip -o "$CORES_DIR/$core" -d "$CORES_DIR"
  rm "$CORES_DIR/$core"
done

echo "Downloading BIOS Pack..."
BIOS_URL="https://archive.org/download/retroarch-bios-pack-202404/RetroArch_BIOS_Pack.zip"
curl -L -H "User-Agent: Mozilla/5.0" "$BIOS_URL" -o "$SYSTEM_DIR/bios.zip"
unzip -o "$SYSTEM_DIR/bios.zip" -d "$SYSTEM_DIR"
rm "$SYSTEM_DIR/bios.zip"

echo "Done!"
