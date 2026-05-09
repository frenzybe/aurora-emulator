# Руководство по разработке эмулятора Mega Drive

## 🎯 Цель этого документа

Помочь новым разработчикам (и вам в будущем) быстро включиться в проект, понять архитектуру и добавить новые функции.

## 📚 Образовательные ресурсы

### Обязательно к изучению (в этом порядке):

1. **Архитектура Mega Drive**
   - [Segaretro.org — Mega Drive](https://segaretro.org/Mega_Drive)
   - [MD Hardware Specs (PDF)](https://segaretro.org/images/9/9f/MegaDrive_Manual.pdf)
   - Особое внимание: Memory map, VDP registers, Controller port

2. **Motorola 68000 CPU**
   - [M68000 Programmer's Reference Manual](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)
   - [68000 Instruction Set](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)
   - Практика: реализуйте арифметику, потом addressing modes

3. **Zilog Z80 CPU**
   - [Z80 User Manual](https://www.zilog.com/docs/z80/um0080.pdf)
   - [Z80 Instruction Set](https://clrhome.org/table/)
   - Важно: понимание прерываний и I/O

4. **VDP (Video Display Processor)**
   - [VDP Documentation](https://www.omgwiki.com/doku.php/documents:assembly:megadrive:start)
   - [VDP Register Reference](https://segaretro.org/VDP)
   - Ключевые темы: tiles, sprites, scrolling, priorities

5. **YM2612 (FM Synthesis)**
   - [YM2612 Manual](https://www.selapa.com/ym2612/)
   - [FM Synthesis Theory](https://www.adventurekid.com/akytutorials.htm)
   - Сложная тема, можно отложить на потом

## 🏗️ Архитектурные принципы

### 1. Модульность
Каждый компонент (CPU, VDP, Audio) — независимый класс с чётким интерфейсом. Изменения в одном не должны ломать другие.

### 2. Точность vs Скорость
- **Сначала сделайте "достаточно точно"** — игра должна запускаться
- **Потом оптимизируйте** — JIT, SIMD, multithreading
- **Cycle-accurate** — только если нужна 100% совместимость

### 3. Тестируемость
- Пишите unit-тесты для каждого компонента
- Используйте тестовые ROM (см. ниже)
- Документируйте known issues

## 📁 Структура кода (детально)

```
include/          # Заголовочные файлы (public API)
  types.hpp       # Базовые типы, константы
  utils.hpp       # Вспомогательные функции
  bus.hpp         # Шина (интерфейс)
  emulator.hpp    # Главный класс
  cpu/            # Процессоры
    m68k.hpp      # Motorola 68000
    z80.hpp       # Zilog Z80
  vdp.hpp         # Video Display Processor
  audio/          # Звук
    psg.hpp       # SN76489
    fm.hpp        # YM2612
    audio.hpp     # Микшер
  input/          # Ввод
    controller.hpp # Базовый контроллер
    input.hpp      # Менеджер
    dualsense.hpp  # PlayStation 5
  renderer/       # Видео
    renderer.hpp   # Абстрактный
    software.hpp   # Software рендерер

src/             # Реализация
  main.cpp       # Точка входа
  emulator.cpp   # Связывание компонентов
  bus.cpp        # Реализация шины
  cpu/           # Реализация CPU
  vdp.cpp        # Реализация VDP
  audio/         # Реализация аудио
  input/         # Реализация ввода
  renderer/      # Реализация рендерера
```

## 🚀 Как начать разработку

### Шаг 1: Настройка окружения

```bash
# Установите зависимости (см. BUILD.md)
# macOS
brew install cmake sdl2 hidapi

# Ubuntu
sudo apt install build-essential cmake libsdl2-dev libhidapi-dev

# Сборка
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
```

### Шаг 2: Запуск "Hello World"

Сейчас эмулятор выводит синий экран. Это нормально — MVP.

**Что должно происходить:**
1. `main.cpp` создаёт `Emulator`
2. `Emulator::init()` создаёт все компоненты
3. `load_rom()` загружает ROM в память
4. `run()` запускает цикл:
   - `process_events()` — обработка SDL событий
   - `frame()` — один кадр:
     - `cpu_68k->run_frame()` — выполнение CPU
     - `vdp->step()` — тайминг VDP
     - `renderer->present()` — вывод на экран

### Шаг 3: Отладка CPU

Добавьте в `src/cpu/m68k.cpp`:

```cpp
#ifdef M68K_DEBUG
    if (reg.pc == 0x0000 || reg.pc == 0x100) {
        std::cout << "PC=0x" << std::hex << reg.pc 
                  << " Opcode=0x" << std::hex << opcode << "\n";
    }
#endif
```

Скомпилируйте с `-DM68K_DEBUG=1` в CMake.

### Шаг 4: Протестировать на простом ROM

Создайте тестовый ROM (или найдите homebrew):
- [Sega TeraDrive Diagnostics](https://github.com/retro-rsm/segateradrive)
- [MD Test ROM](https://segaretro.org/Mega_Drive_Checker)

Поместите ROM в папку `tests/roms/` и запустите:
```bash
./MegaDriveEmu tests/roms/md_checker.bin
```

## 🔧 Добавление новых инструкций CPU

### Для 68000:

1. **Найдите инструкцию** в [M68000 PRM](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)
2. **Определите opcode** (биты 15-0)
3. **Декодируйте** в `decode_execute()`:
   ```cpp
   case 0xXXXX: // opcode
       // Реализация
       break;
   ```
4. **Реализуйте**:
   - Fetch operands (используйте `get_data()`)
   - Выполните операцию
   - Update flags (`update_flags_*()`)
   - Add cycles (`add_cycles(N)`)
5. **Протестируйте** на изолированном тесте

### Пример: ADD immediate (word)

```cpp
case 0x0640: // ADD.W #<data>,Dn
    {
        u16 imm = fetch_word();
        u32 a = reg.d[reg];
        bool carry, overflow;
        u32 result = add_u32(a, imm, carry, overflow);
        reg.d[reg] = result;
        update_flags_add(result, a, imm, 16);
        add_cycles(8);
    }
    break;
```

### Для Z80:

Аналогично, но проще (8-bit). Смотрите `src/cpu/z80.cpp`, там уже есть шаблон.

## 🎨 Рендеринг графики

### Текущее состояние:
- VDP инициализируется
- `render_scanline()` вызывается для каждой строки
- Сейчас рисует синий фон

### Что нужно сделать:

1. **Загрузить tile data в VRAM** (через DMA или CPU write)
2. **Реализовать tile map rendering**:
   ```cpp
   void VDP::render_tile(int x, int y, const Tile& tile, ...) {
       // Вычислить адрес tile в VRAM
       // Прочитать 8x8 pixels (4 bpp = 32 bytes)
       // Конвертировать palette index → RGB
       // Записать в line_buffer
   }
   ```
3. **Отрисовка спрайтов** (после tile map, с приоритетами)
4. **Scrolling** (изменение base address plane A/B)

### Tile format (Mega Drive):
- 8x8 pixels
- 4 bits per pixel (16 colors per tile)
- 32 bytes per tile
- Tile data: planar (2 bit planes × 4 planes)

### Palette:
- 512 colors (9-bit: 3R+3G+3B)
- 64 on-screen simultaneously (with shadow/highlight)

## 🔊 Звуковая подсистема

### PSG (SN76489) — уже работает!

**Как тестировать:**
- Запишите в PSG регистры через I/O порт 0x00
- Установите tone и volume
- `psg->tick()` генерирует сэмпл

**Что добавить:**
- Noise channel (упрощённо уже есть)
- Proper frequency calculation: `freq = clock / (32 * (tone+1))`
- Volume envelope (PSG имеет 4-bit volume + attenuation)

### FM (YM2612) — заглушка

**Сложная часть!** Потребует времени.

**Пошагово:**
1. Реализуйте все 20 регистров оператора
2. Phase accumulation (основной цикл)
3. Envelope generator (ADSR)
4. Operator connection (алгоритмы 1-4)
5. LFO
6. Output mixing

**Совет:** Сначала сделайте простой sine wave, потом усложняйте.

## 🎮 Система ввода

### Текущее:
- SDL2 GameController API (Xbox, Switch Pro)
- Клавиатура (заглушка)
- DualSense (hidapi, частично)

### Что добавить:

1. **Завершите DualSense:**
   - Полный парсинг input report (64 байта)
   - Отправка output report (управление триггерами, LED)
   - Тест на реальном железе

2. **Профили маппинга:**
   ```json
   {
     "game": "Sonic the Hedgehog 2",
     "mappings": {
       "A": "CROSS",
       "B": "CIRCLE",
       ...
     }
   }
   ```

3. **6-button controller support:**
   - Mega Drive поддерживал 6-button pads
   - Нужно эмулировать последовательность чтения

## ⚡ Оптимизация (позже)

### JIT-компиляция

**Почему:** Интерпретатор 68000 слишком медленный (~1-5 MIPS, нужно ~10 MIPS).

**Как:**
1. **Basic block detection:**
   - Собирайте последовательности инструкций до branch/jump
   - Кэшируйте скомпилированный код

2. **Translation:**
   - Каждая 68000 инструкция → несколько x86_64 инструкций
   - Используйте AsmJit или DynASM

3. **Invalidation:**
   - При self-modifying code — инвалидируйте кэш

**Пример упрощённого JIT:**
```cpp
class JITCompiler {
    std::unordered_map<u32, void*> cache;
    
    void* compile_block(u32 start_pc) {
        // Собираем block
        // Генерируем x86_64 код
        // Возвращаем указатель на функцию
    }
    
    void execute(void* func) {
        // Вызываем скомпилированный код
    }
};
```

### Multithreading

```
Main Thread:    CPU emulation (68K)
Render Thread:  VDP → OpenGL
Audio Thread:   PSG/FM → SDL Audio
Input Thread:   SDL events
```

**Синхронизация:** lock-free queues, atomic operations.

### SIMD

Используйте SSE/AVX/NEON для:
- Blitting (копирование пикселей)
- Audio resampling
- Memory operations (memset, memcpy)

## 🧪 Тестирование

### Unit-тесты (Google Test)

```cpp
TEST(CPU68KTest, AddImmediate) {
    CPU68K cpu(&bus);
    cpu.reg.d[0] = 0x10;
    // Execute ADD.W #0x20, D0
    // Check result = 0x30, flags correct
}
```

### Интеграционные тесты

Запускайте реальные ROM и проверяйте:
- Доходит ли до V-Blank?
- Есть ли звук?
- Кнопки работают?

### Регрессионное тестирование

Создайте `tests/` с известными ROM и ожидаемым поведением. После каждого коммита запускайте все тесты.

## 📝 Style guide

Следуйте `.clang-format`:
- 4 пробела, нет табов
- Имена: `camelCase` для переменных, `PascalCase` для классов
- Файлы: `snake_case.cpp`, `snake_case.hpp`
- Пространства имён: `namespace md { ... }`

## 🔍 Отладка

### GDB/Lldb

```bash
gdb ./MegaDriveEmu
(gdb) break src/cpu/m68k.cpp:123
(gdb) run rom.bin
(gdb) info registers
(gdb) x/10i $pc
```

### Логи

Включите логи в коде:
```cpp
#define LOG_LEVEL 3  // 0=none, 1=error, 2=warn, 3=info, 4=debug
#include "logger.hpp"
LOG_DEBUG("PC=0x%08X", reg.pc);
```

### Визуализация

Добавьте в renderer:
- Отображение VRAM как tile viewer
- Показ sprite list
- Индикатор V-Blank/H-Blank

## 🐛 Известные проблемы

### 1. CPU не доходит до V-Blank
**Причина:** Не хватает инструкций, неправильные циклы.
**Решение:** Добавьте недостающие инструкции, проверьте cycle counting.

### 2. Нет графики
**Причина:** VDP не рендерит спрайты/тайлы.
**Решение:** Реализуйте `render_tile()` и `render_sprite()`.

### 3. Звук хрипит
**Причина:** Неправильная частота дискретизации.
**Решение:** PSG: `clock / 16 / (tone+1)`, FM: более сложная математика.

### 4. Контроллер не работает
**Причина:** I/O порты не маппятся правильно.
**Решение:** Проверьте `bus.cpp` read_io/write_io для портов 0x04-0x05.

## 📈 Roadmap разработки

### Неделя 1-2: MVP
- [ ] Сборка без ошибок
- [ ] Загрузка ROM
- [ ] Синий экран (уже есть)
- [ ] Обработка клавиатуры

### Неделя 3-4: Первая игра
- [ ] Достаточно инструкций 68000 для Sonic 1
- [ ] VDP: tiles + scrolling plane
- [ ] Звук: PSG работает
- [ ] Тест: Sonic 1 заглавный экран

### Месяц 2: Совместимость
- [ ] Полный 68000 ISA
- [ ] Полный Z80 ISA
- [ ] VDP: sprites, priorities
- [ ] FM: базовая эмуляция
- [ ] 6-button controllers
- [ ] SRAM saves

### Месяц 3: Оптимизация
- [ ] JIT-компилятор (начало)
- [ ] OpenGL рендерер
- [ ] 60 FPS стабильно
- [ ] Улучшенный UI

### Месяц 4+: DualSense
- [ ] Полная поддержка hidapi
- [ ] Адаптивные триггеры
- [ ] HD haptic
- [ ] RGB LED
- [ ] Профили per-game

## 🎓 Что изучать дальше

После завершения MVP:

1. **JIT-компиляция:** 
   - Книга: "Writing an Interpreter in Go" (принципы)
   - AsmJit documentation
   - DynASM from LuaJIT

2. **Cycle-accurate эмуляция:**
   - Изучите точные тайминги Mega Drive
   - Bus contention, waitstates
   - Смотрите код Blastem

3. **Оптимизация:**
   - "Computer Systems: A Programmer's Perspective"
   - Профилирование: perf, VTune, Instruments

4. **Графика:**
   - OpenGL/Vulkan tutorials
   - Шейдеры: CRT-Lottes, xBRZ
   - Уроки от RetroArch

## 🤝 Как внести вклад

1. **Выберите задачу** из [Issues](https://github.com/your-repo/issues)
2. **Создайте ветку:** `git checkout -b feature/name`
3. **Реализуйте** с соблюдением style guide
4. **Протестируйте** на нескольких ROM
5. **Создайте PR** с описанием изменений

### Нужные области:
- **68000 CPU:** ~980 инструкций осталось
- **VDP:** tiles, sprites, scrolling, windows
- **YM2612:** полная FM синтез
- **DualSense:** hidapi, advanced features
- **JIT:** динамическая компиляция
- **UI:** ImGui/Qt интерфейс
- **Тесты:** unit + integration tests

## 📚 Дополнительные материалы

### Код существующих эмуляторов (учитесь!)
- **Genesis Plus GX** — эталон архитектуры
- **Kega Fusion** — эталон точности
- **Blastem** — эталон производительности
- **MAME** — полная, но сложная

### Книги
- "The Ultimate Game Developer" — низкоуровневая графика
- "Game Programming Patterns" — архитектура
- "Computer Systems: A Programmer's Perspective" — оптимизация

### Видео
- "Making an emulator" — серии на YouTube
- "Emulator 101" — конференции
- "Cycle-exact emulation" — глубокие технические доклады

## 🎯 Критерии качества кода

- [ ] **Компилируется без ошибок/предупреждений** (`-Werror`)
- [ ] **Проходит тесты** (если есть)
- [ ] **Соблюдает style guide** (`.clang-format`)
- [ ] **Документирован** (комментарии в коде, Doxygen-style)
- [ ] **Производителен** (не тормозит)
- [ ] **Совместим** (работает на Windows/macOS/Linux)

## 🚀 Быстрый старт для новых разработчиков

1. **Читайте этот документ** целиком
2. **Изучите архитектуру** (PROJECT_SUMMARY.md)
3. **Соберите проект** (BUILD.md)
4. **Запустите** с тестовым ROM
5. **Поставьте точку останова** в `m68k.cpp:step()`
6. **Пошагово отладьте** одну инструкцию
7. **Добавьте новую инструкцию** (см. раздел выше)
8. **Протестируйте** на простом ROM
9. **Commit** с понятным сообщением
10. **Создайте PR**

---

**Удачи в разработке!** 🚀

Вопросы? Создайте Issue в репозитории.
