#!/bin/bash
CORES_DIR="src-tauri/resources/cores"
BASE_URL="https://buildbot.libretro.com/nightly/apple/osx/arm64/latest"

echo "Downloading ALL Libretro Cores..."
cores=(
  "fceumm_libretro.dylib.zip"
  "snes9x_libretro.dylib.zip"
  "genesis_plus_gx_libretro.dylib.zip"
  "pcsx_rearmed_libretro.dylib.zip"
  "mupen64plus_next_libretro.dylib.zip"
  "gambatte_libretro.dylib.zip"
  "mgba_libretro.dylib.zip"
  "desmume_libretro.dylib.zip"
  "ppsspp_libretro.dylib.zip"
  "flycast_libretro.dylib.zip"
  "dolphin_libretro.dylib.zip"
  "dosbox_pure_libretro.dylib.zip"
  "yabause_libretro.dylib.zip"
  "opera_libretro.dylib.zip"
  "stella_libretro.dylib.zip"
  "pcsx2_libretro.dylib.zip"
  "citra_libretro.dylib.zip"
  "fmsx_libretro.dylib.zip"
  "puae_libretro.dylib.zip"
  "scummvm_libretro.dylib.zip"
  "mednafen_wswan_libretro.dylib.zip"
  "mednafen_ngp_libretro.dylib.zip"
  "beetle_vb_libretro.dylib.zip"
  "o2em_libretro.dylib.zip"
  "fbneo_libretro.dylib.zip"
)

for core in "${cores[@]}"; do
  if [ ! -f "$CORES_DIR/${core%.zip}" ]; then
    echo "Fetching $core..."
    curl -L "$BASE_URL/$core" -o "$CORES_DIR/$core"
    if [ -f "$CORES_DIR/$core" ]; then
      unzip -o "$CORES_DIR/$core" -d "$CORES_DIR"
      rm "$CORES_DIR/$core"
    fi
  else
    echo "Core ${core%.zip} already exists, skipping."
  fi
done

echo "All cores ready!"
