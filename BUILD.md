# Инструкции по сборке и запуску

## 🚀 Быстрый старт

### 1. Установите зависимости

**macOS:**
```bash
brew install cmake sdl2 hidapi
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libhidapi-dev
```

**Windows (с vcpkg):**
```powershell
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg install sdl2 hidapi
```

### 2. Сборка

```bash
# Создание build директории
mkdir build && cd build

# Генерация Makefiles (или CMake может сгенерировать для вашей IDE)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Сборка
cmake --build . -j$(nproc)  # Linux/macOS
# или
cmake --build . --config Release  # Windows
```

### 3. Запуск

```bash
# Сначала нужен ROM Mega Drive
./MegaDriveEmu /path/to/your/game.bin
```

**Пример:**
```bash
./MegaDriveEmu ~/Games/SonicTheHedgehog.bin
```

## 🔧 Настройка CMake

### Опции сборки

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTS=ON \
  -DENABLE_PROFILING=OFF
```

**Опции:**
- `CMAKE_BUILD_TYPE`: `Debug`, `Release`, `RelWithDebInfo`
- `ENABLE_TESTS`: Включить юнит-тесты (по умолчанию ON)
- `ENABLE_PROFILING`: Включить инструментирование для профилирования (Tracy, perf)

### Выбор компилятора

```bash
# GCC
cmake .. -DCMAKE_CXX_COMPILER=g++-12

# Clang
cmake .. -DCMAKE_CXX_COMPILER=clang++-15

# MSVC (через Visual Studio)
cmake .. -G "Visual Studio 17 2022" -A x64
```

## 🐛 Отладка

### 1. Сборка в Debug режиме

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### 2. Запуск под gdb/lldb

```bash
gdb ./MegaDriveEmu
(gdb) run /path/to/rom.bin
(gdb) break src/cpu/m68k.cpp:123
(gdb) continue
```

### 3. Логирование

В коде используйте:
```cpp
#include <iostream>
std::cout << "Debug message\n";
```

Или подключите spdlog для продвинутого логирования.

## 📊 Профилирование

### Linux (perf)
```bash
perf record -g ./MegaDriveEmu game.bin
perf report
```

### macOS (Instruments)
```bash
instruments -t "Time Profiler" ./MegaDriveEmu
```

### Windows (Visual Studio Profiler)
Откройте решение в VS и используйте Performance Profiler.

## 🧪 Тестирование

### Юнит-тесты (Google Test)

```bash
cd build
ctest --verbose
# или
./tests/unit_tests
```

### Тестовые ROM

Поместите тестовые ROM в папку `tests/roms/`:
- `md_checker.bin` — диагностика
- `sonic1.bin` — базовый тест
- `streets_of_rage2.bin` — стресс-тест

## 🎯 Отладка эмуляции

### Включение логов VDP

Добавьте в `src/vdp/vdp.cpp`:
```cpp
#define VDP_DEBUG 1
```

### Включение логов CPU

В `src/cpu/m68k.cpp`:
```cpp
#define M68K_DEBUG 1
```

### Dump памяти

```cpp
// В нужном месте
for (int i = 0; i < 16; i++) {
    printf("%04X: %02X %02X %02X %02X\n", 
           addr + i*4, 
           bus->read(addr + i*4 + 0),
           bus->read(addr + i*4 + 1),
           bus->read(addr + i*4 + 2),
           bus->read(addr + i*4 + 3));
}
```

## 🎮 Добавление поддержки DualSense

### 1. Установите hidapi

```bash
# macOS
brew install hidapi

# Ubuntu
sudo apt install libhidapi-dev
```

### 2. Соберите с поддержкой HID

```bash
cmake .. -DENABLE_HIDAPI=ON
```

### 3. Подключите DualSense через USB/Bluetooth

Запустите эмулятор — контроллер должен определиться автоматически.

## 🐛 Известные проблемы

### Проблема: Не запускается, segmentation fault
**Решение:** Убедитесь, что SDL2 установлен и найден CMake.

### Проблема: Нет звука
**Решение:** Звуковая подсистема в разработке. Пока отключите в `main.cpp`.

### Проблема: Медленно работает
**Решение:** JIT-компилятор ещё не реализован. Ожидайте ускорения в будущих версиях.

## 📈 Мониторинг прогресса

Смотрите `PROGRESS.md` (будет обновляться) для отслеживания:
- Совместимость с играми
- Производительность
- Реализованные функции

## 🆘 Нужна помощь?

1. Проверьте [Issues](https://github.com/your-repo/issues)
2. Создайте новый Issue с описанием проблемы
3. Приложите логи и информацию о системе:

```bash
# Информация о системе
uname -a  # Linux/macOS
systeminfo  # Windows

# Версия CMake
cmake --version

# Версия компилятора
g++ --version  # или clang++ --version
```

---

**Удачи в сборке!** 🚀
