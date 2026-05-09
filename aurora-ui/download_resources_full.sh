#!/bin/bash
CORES_DIR="src-tauri/resources/cores"
SYSTEM_DIR="src-tauri/resources/system"
BASE_URL="https://buildbot.libretro.com/nightly/apple/osx/arm64/latest"

echo "Downloading Missing Cores..."
cores=(
  "beetle_saturn_libretro.dylib.zip"
  "opera_libretro.dylib.zip"
  "stella_libretro.dylib.zip"
  "dosbox_pure_libretro.dylib.zip"
  "fbneo_libretro.dylib.zip"
  "flycast_libretro.dylib.zip"
  "dolphin_libretro.dylib.zip"
  "desmume_libretro.dylib.zip"
)

for core in "${cores[@]}"; do
  echo "Fetching $core..."
  curl -L "$BASE_URL/$core" -o "$CORES_DIR/$core"
  if [ -f "$CORES_DIR/$core" ]; then
    unzip -o "$CORES_DIR/$core" -d "$CORES_DIR"
    rm "$CORES_DIR/$core"
  fi
done

echo "Attempting BIOS Download again..."
# Alternative source for some main BIOS files
curl -L -H "User-Agent: Mozilla/5.0" "https://archive.org/download/retroarch-bios-pack-202404/RetroArch_BIOS_Pack.zip" -o "$SYSTEM_DIR/bios.zip"
if [ -f "$SYSTEM_DIR/bios.zip" ]; then
    unzip -o "$SYSTEM_DIR/bios.zip" -d "$SYSTEM_DIR"
    rm "$SYSTEM_DIR/bios.zip"
fi

touch "$SYSTEM_DIR/.keep"
echo "Done!"
