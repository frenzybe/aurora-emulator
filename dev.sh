#!/bin/bash

# 1. Build the Emulator Core (C++)
echo "==> Building Emulator Core..."
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
cd ..

# 2. Detect Architecture and Update Sidecar
ARCH=$(uname -m)
if [ "$ARCH" == "arm64" ]; then
    TRIPLE="aarch64-apple-darwin"
else
    TRIPLE="x86_64-apple-darwin"
fi

echo "==> Updating Sidecar binary for $TRIPLE..."
mkdir -p aurora-ui/src-tauri/binaries
cp build/MegaDriveEmu aurora-ui/src-tauri/binaries/MegaDriveEmu-$TRIPLE

# 3. Launch Tauri Dev
echo "==> Launching Aurora UI..."
cd aurora-ui
npm run tauri dev
