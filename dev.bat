@echo off
set VCPKG_PATH=C:/vcpkg

echo [1/4] Building C++ Core...
mkdir build
cd build
cmake .. -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
if %errorlevel% neq 0 exit /b %errorlevel%

echo [2/4] Setting up Sidecar...
mkdir ..\aurora-ui\src-tauri\binaries
copy Release\MegaDriveEmu.exe ..\aurora-ui\src-tauri\binaries\MegaDriveEmu-x86_64-pc-windows-msvc.exe

echo [3/4] Installing UI dependencies...
cd ..\aurora-ui
call npm install

echo [4/4] Building Aurora Launcher...
call npm run tauri build

echo DONE! Your installers are in aurora-ui/src-tauri/target/release/bundle/
pause
