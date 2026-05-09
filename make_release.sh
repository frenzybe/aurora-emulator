#!/bin/bash
set -e

# 1. Сборка эмулятора (Release)
echo "🚀 Сборка эмулятора в режиме Release..."
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
cd ..

# 2. Подготовка сайдкара (Sidecar)
echo "📦 Подготовка сайдкара для текущей платформы..."
# Автоматически определяем архитектуру (aarch64-apple-darwin или x86_64-apple-darwin)
TRIPLE=$(rustc -Vv | grep host | cut -d ' ' -f 2)
mkdir -p aurora-ui/src-tauri/binaries

if [ -f "build/MegaDriveEmu" ]; then
    cp build/MegaDriveEmu aurora-ui/src-tauri/binaries/MegaDriveEmu-$TRIPLE
elif [ -f "build/Release/MegaDriveEmu.exe" ]; then
    cp build/Release/MegaDriveEmu.exe aurora-ui/src-tauri/binaries/MegaDriveEmu-$TRIPLE.exe
elif [ -f "build/MegaDriveEmu.exe" ]; then
    cp build/MegaDriveEmu.exe aurora-ui/src-tauri/binaries/MegaDriveEmu-$TRIPLE.exe
fi

# 3. Сборка лаунчера (Tauri)
echo "🎨 Сборка лаунчера Aurora..."
cd aurora-ui
npm install
npx tauri build

echo "✅ ГОТОВО! Ваш установщик находится в папке: aurora-ui/src-tauri/target/release/bundle"
