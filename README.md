# Mega Drive Emulator

Эмулятор Sega Mega Drive/Genesis с поддержкой современных контроллеров, включая DualSense.

## 🎮 Особенности

- **Полная эмуляция** Mega Drive/Genesis (Motorola 68000 + Z80)
- **Современные контроллеры**: DualSense, Xbox, Switch Pro Controller
- **High-definition haptic** feedback (DualSense)
- **Адаптивные триггеры** (опционально)
- **Кроссплатформенность**: Windows, macOS, Linux
- **Software рендерер** (пока что, потом OpenGL/Vulkan)
- **Точная эмуляция** звука (PSG + FM)

## 🏗️ Архитектура

```
MegaDriveEmu/
├── CMakeLists.txt          # Сборка
├── src/                    # Исходники
│   ├── main.cpp           # Точка входа
│   ├── emulator.cpp       # Главный класс эмулятора
│   ├── bus.cpp            # Шина памяти/I/O
│   ├── cpu/               # Процессоры
│   │   ├── m68k.cpp       # Motorola 68000
│   │   └── z80.cpp        # Zilog Z80
│   ├── vdp/               # Video Display Processor
│   ├── audio/             # Звуковая подсистема
│   │   ├── psg.cpp        # SN76489
│   │   ├── fm.cpp         # YM2612
│   │   └── audio.cpp      # Микшер
│   ├── input/             # Система ввода
│   │   ├── input.cpp      # Менеджер
│   │   ├── controller.cpp # Базовый контроллер
│   │   └── dualsense.cpp  # PlayStation 5
│   └── renderer/          # Видео
│       └── software.cpp   # Software рендерер
└── include/               # Заголовки
```

## 📦 Зависимости

- **CMake** 3.16+
- **C++20** компилятор (GCC 10+, Clang 12+, MSVC 2019+)
- **SDL2** (для окна, ввода, аудио)
- **hidapi** (для DualSense, опционально)

### Установка зависимостей

**macOS:**
```bash
brew install cmake sdl2 hidapi
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake g++ libsdl2-dev libhidapi-dev
```

**Windows:**
- Установите [vcpkg](https://github.com/microsoft/vcpkg)
- `vcpkg install sdl2 hidapi`

## 🔨 Сборка

```bash
# Клонируйте репозиторий
git clone <your-repo>
cd megadrive-emu

# Создание build директории
mkdir build && cd build

# Конфигурация CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Сборка
cmake --build . -j$(nproc)

# Запуск
./MegaDriveEmu /path/to/rom.bin
```

## 🎯 Текущий статус

### Реализовано ✅
- [x] Базовая архитектура эмулятора
- [x] Шина памяти (Bus)
- [x] Motorola 68000 (интерпретатор, часть инструкций)
- [x] Zilog Z80 (интерпретатор, часть инструкций)
- [x] VDP (инициализация, регистры)
- [x] PSG (SN76489, базовый звук)
- [x] FM (YM2612, заглушка)
- [x] Audio mixer
- [x] Input manager (SDL)
- [x] Software renderer (SDL2)
- [x] Главный цикл эмуляции

### В разработке 🚧
- [ ] Полный набор инструкций 68000
- [ ] Полный набор инструкций Z80
- [ ] Точная эмуляция VDP (tiles, sprites, scrolling)
- [ ] Полная эмуляция YM2612 (FM synthesis)
- [ ] JIT-компиляция 68000 → x86_64
- [ ] Поддержка DualSense через hidapi
- [ ] Адаптивные триггеры и HD haptic
- [ ] Сохранения (SRAM, save states)
- [ ] Шейдеры (CRT, xBRZ upscaling)

### Планируется 📋
- [ ] Mega CD поддержка (опционально)
- [ ] 32X поддержка (опционально)
- [ ] Netplay (rollback netcode)
- [ ] Debugger/disassembler
- [ ] Cheat system (Game Genie)

## 🎮 Управление

### Стандартное (клавиатура)
- **Стрелки** — D-Pad
- **Z** — A (Cross)
- **X** — B (Circle)
- **C** — C (Square)
- **V** — X (Triangle)
- **A** — Y (L1)
- **S** — Z (R1)
- **Enter** — Start

### Современные геймпады
- Автоопределение через SDL2
- Маппинг по умолчанию:
  - **A** → Cross (✕)
  - **B** → Circle (○)
  - **X** → Square (□)
  - **Y** → Triangle (△)
  - **L1/R1** → Y/Z

### DualSense (PS5)
- Полная поддержка через hidapi
- Адаптивные триггеры (настраиваются)
- HD haptic feedback
- RGB подсветка (интеграция с игрой)

## 📖 Ресурсы для разработки

### Документация
- [Mega Drive Hardware Manual](https://segaretro.org/Mega_Drive)
- [Motorola 68000 Programmer's Reference](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)
- [VDP Documentation](https://www.omgwiki.com/doku.php/documents/assembly:megadrive:start)
- [YM2612 (OPN2) Manual](https://www.selapa.com/ym2612/)

### Тестовые ROM
- [Sega TeraDrive Diagnostics](https://github.com/retro-rsm/segateradrive)
- [Mega Drive Checker](https://segaretro.org/Mega_Drive_Checker)
- [Sonic 1/2 Test ROMs](https://s2emu.s2dev.net/)

### Существующие эмуляторы (для изучения)
- **Genesis Plus GX** — чистый C, отличная архитектура
- **Kega Fusion** — закрытый, но эталон точности
- **Blastem** — фокус на производительность

## 🗺️ Roadmap

**Этап 1 (MVP) — 6 месяцев**
- [ ] Запуск простых ROM (демо, homebrew)
- [ ] Базовый ввод (клавиатура, геймпады)
- [ ] Звук (PSG + FM)
- [ ] Сохранения (SRAM)

**Этап 2 — 12 месяцев**
- [ ] Полная совместимость с 90% ROM
- [ ] JIT-компилятор (10x ускорение)
- [ ] OpenGL рендерер
- [ ] Поддержка 6-button controllers

**Этап 3 — 18 месяцев**
- [ ] Улучшенный UI (ImGui)
- [ ] Шейдеры (CRT, upscaling)
- [ ] Save states
- [ ] Поддержка Mega CD (опционально)

**Этап 4 — 24 месяца**
- [ ] Netplay
- [ ] Debugger
- [ ] Cheat engine
- [ ] Полная DualSense интеграция

## 🤝 Вклад в проект

Вклад приветствуется! Сначала создайте Issue с описанием, затем PR.

### Areas needing help:
- **68000 CPU**: полная ISA, точность тайминга
- **VDP**: tiles, sprites, scrolling, priorities
- **YM2612**: точная FM синтез
- **DualSense**: hidapi, adaptive triggers, haptic
- **JIT**: динамическая компиляция
- **UI**: Qt/ImGui интерфейс

## ⚖️ Лицензия

Это учебный проект. Код под лицензией MIT.

**Юридические примечания:**
- BIOS и ROMы не входят в репозиторий
- Используйте только легальные копии
- Эмулятор только для личного использования

## 📞 Контакты

Вопросы? Создайте Issue в репозитории.

---

**Happy Emulating!** 🎮
