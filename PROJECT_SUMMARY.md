# Проект: Эмулятор Sega Mega Drive/Genesis

## 📋 Обзор

**Название:** MegaDriveEmu  
**Платформа:** Sega Mega Drive/Genesis (1988-1997)  
**Язык:** C++20  
**Лицензия:** MIT  

**Ключевая особенность:** Поддержка современных контроллеров, включая PlayStation DualSense с адаптивными триггерами и HD haptic feedback.

## 🎯 Цели проекта

### Основная цель
Создать точный, производительный эмулятор Mega Drive с уникальной поддержкой современных игровых контроллеров.

### Уникальные фичи (USP)
1. **DualSense Integration** — полная поддержка PlayStation 5 контроллера:
   - Адаптивные триггеры (эмуляция сопротивления)
   - HD haptic (направленная вибрация)
   - RGB-подсветка (интеграция с игрой)
   - Touchpad как дополнительные кнопки
   - Гироскоп для motion-control игр

2. **Modern Controller Support** — plug-and-play:
   - Xbox Series X|S Controller
   - Nintendo Switch Pro Controller
   - Любой SDL2-совместимый геймпад

3. **Производительность**:
   - JIT-компиляция 68000 → x86_64/ARM
   - Multithreading (CPU/GPU/audio отдельно)
   - SIMD-оптимизации

## 🏛️ Архитектура (детально)

### Модульная структура

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                    │
│  • Главное окно (SDL2)                                  │
│  • Меню, настройки, UI (позже ImGui)                   │
│  • Загрузка ROM/BIOS                                    │
├─────────────────────────────────────────────────────────┤
│                Emulation Core (ядро)                    │
├─────────────┬─────────────┬─────────────┬───────────────┤
│   CPU68K    │   CPUZ80    │     VDP     │    Audio      │
│  (Main)     │  (Sound)    │  (Video)    │  (PSG+FM)     │
├─────────────┴─────────────┴─────────────┴───────────────┤
│              Bus (Memory & I/O)                        │
│  • 24-bit address space (16MB)                         │
│  • Memory mapping (ROM, RAM, VRAM, I/O)               │
│  • DMA controller                                      │
│  • Interrupt controller                                │
├─────────────────────────────────────────────────────────┤
│           Peripheral Subsystems                        │
│  • Input Manager (SDL + hidapi)                        │
│  • Renderer (Software → OpenGL → Vulkan)               │
│  • Audio Backend (SDL2 audio / PortAudio)              │
│  • Save State Manager                                  │
└─────────────────────────────────────────────────────────┘
```

### Ключевые компоненты

#### 1. Bus (Шина)
- **Ответственность:** Memory-mapped I/O, DMA, прерывания
- **Память:**
  - 64KB RAM (0x000000-0x00FFFF)
  - 64KB VRAM (0x000000-0x00FFFF, отдельно)
  - 8KB Z80 RAM (0x000000-0x01FFF)
  - ROM (до 4MB, 0x000000-0x3FFFFF)
- **I/O порты:** 0xA00000-0xA0FFFF, 0xC00000-0xC0003F

#### 2. CPU68K (Motorola 68000)
- **Тактовая:** 7.67 MHz (PAL) / 7.89 MHz (NTSC)
- **Архитектура:** 16/32-bit CISC, 24-bit address bus
- **Регистры:** 8 data (D0-D7) + 8 address (A0-A7) + SR
- **Режимы адресации:** 14 режимов (immediate, absolute, indexed, etc.)
- **Инструкции:** ~1000 (полный набор 68000)
- **Эмуляция:** Интерпретатор → JIT (позже)

#### 3. CPUZ80 (Zilog Z80)
- **Тактовая:** 3.58 MHz (отдельный кристалл)
- **Роль:** Управление звуком (PSG, FM), совместимость с Master System
- **Память:** 8KB внутренняя RAM + доступ к 68K RAM
- **Синхронизация:** Shared memory, waitstates

#### 4. VDP (Video Display Processor)
- **Тип:** Texas Instruments TMS9918-совместимый, но расширенный
- **Память:** 64KB VRAM, 128-word CRAM (512 colors), 64-word VSRAM
- **Режимы:**
  - Mode 4: 256x192, 32 sprites, 2 scrolling planes
  - Mode 5: 256x192, 80 sprites, 4 planes
- **Ограничения:**
  - 20 sprites per line
  - 64 colors on screen (из 512)
  - 8x8 tile-based
- **DMA:** VDP DMA для быстрой загрузки VRAM

#### 5. Audio Subsystem
```
┌─────────────────────────────────────────┐
│  SN76489 (PSG)                          │
│  • 3 square wave channels               │
│  • 1 noise channel                      │
│  • 4-bit volume per channel             │
├─────────────────────────────────────────┤
│  YM2612 (FM)                            │
│  • 6 FM channels (4-op)                 │
│  • 3 SSG channels                       │
│  • 1 ADPCM channel                      │
│  • LFO, алгоритмы связи операторов      │
├─────────────────────────────────────────┤
│  Audio Mixer                            │
│  • Сведение PSG + FM                    │
│  • Resampling до 44.1/48 kHz            │
│  • Глобальная громкость                 │
└─────────────────────────────────────────┘
```

#### 6. Input Subsystem (Ваша киллер-фича!)
```
┌─────────────────────────────────────────────┐
│         Input Manager                       │
├─────────────────────────────────────────────┤
│  • HID Abstraction Layer                    │
│    ├─ SDL2 GameController (Xbox, Switch)   │
│    ├─ hidapi (DualSense specifics)         │
│    └─ Keyboard fallback                     │
│                                             │
│  • Controller Profile System                │
│    ├─ Автоопределение игры (ROM CRC)       │
│    ├─ JSON-профили маппинга                │
│    └─ Настройка чувствительности, deadzone │
│                                             │
│  • DualSense Advanced Features              │
│    ├─ Adaptive Triggers (L2/R2)            │
│    ├─ HD Haptic (направленная вибрация)    │
│    ├─ Touchpad → 2 дополнительные кнопки   │
│    ├─ Gyro (для motion-control игр)        │
│    └─ RGB LED (интеграция с событиями)     │
│                                             │
│  • Rumble Engine                            │
│    ├─ Classic vibration (MD → modern)      │
│    └─ Enhanced haptic (DualSense only)     │
└─────────────────────────────────────────────┘
```

### Data Flow

```
ROM → Bus → CPU68K (интерпретация/JIT) → Memory/I/O
                                    ↓
                              VDP (рендеринг)
                                    ↓
                              Renderer (SDL2)
                                    ↓
                              Окно/Экран

Z80 → PSG/FM → Audio Mixer → SDL Audio → Динамики

Input: Controller → SDL/hidapi → InputManager → Bus (I/O порты) → CPU
```

## 📊 Оценка сложности

| Компонент | Сложность (1-10) | Время (мес) | Статус |
|-----------|-----------------|-------------|--------|
| Bus | 4 | 0.5 | ✅ Готов |
| 68000 Interpreter | 6 | 2 | 🚧 Частично |
| 68000 JIT | 9 | 3 | ⏳ Планируется |
| Z80 Interpreter | 5 | 1 | 🚧 Частично |
| VDP (sprites/planes) | 7 | 2.5 | 🚧 Базово |
| VDP (DMA, windows) | 8 | 1.5 | ⏳ Планируется |
| PSG (SN76489) | 4 | 0.5 | ✅ Готов |
| FM (YM2612) | 6 | 2 | 🚧 Заглушка |
| Audio mixing | 5 | 0.5 | ✅ Готов |
| Input (MD controllers) | 3 | 0.5 | ✅ Готов |
| **DualSense integration** | **4** | **1.5** | ⏳ Планируется |
| Software renderer | 5 | 1 | ✅ Готов |
| OpenGL/Vulkan renderer | 7 | 2 | ⏳ Планируется |
| JIT оптимизации | 9 | 2 | ⏳ Планируется |
| UI (ImGui/Qt) | 6 | 1.5 | ⏳ Планируется |
| **Итого (MVP):** | | **~12 месяцев** | |

**MVP (минимально рабочая версия):** 6-8 месяцев

## 🗺️ Детальный план разработки

### Фаза 0: Подготовка (неделя 1-2) ✅
- [x] Настройка CMake, структура проекта
- [x] Создание заголовочных файлов
- [x] Базовая реализация Bus, типы
- [ ] Настройка CI/CD (GitHub Actions)
- [ ] Добавление тестов (Google Test)

### Фаза 1: Прототип (месяцы 1-3) 🚧
**Цель:** Загрузить ROM, вывести "Hello World" на экран

- [x] Motorola 68000 — 20 базовых инструкций
- [x] Z80 — 50 базовых инструкций
- [x] VDP — инициализация, регистры
- [x] Software renderer — 320x224
- [x] Input — клавиатура
- [ ] **Тест:** Homebrew "Hello World" ROM

### Фаза 2: Базовый функционал (месяцы 4-6)
**Цель:** Запуск простых коммерческих игр

- [ ] Полный 68000 ISA (~1000 инструкций)
- [ ] Полный Z80 ISA (~1500 инструкций)
- [ ] VDP — спрайты, scrolling planes
- [ ] PSG — точная эмуляция SN76489
- [ ] FM — базовая эмуляция YM2612
- [ ] Controller port — 3-button gamepad
- [ ] SRAM — сохранения
- [ ] **Тест:** Sonic the Hedgehog 1, Alex Kidd

### Фаза 3: Улучшения (месяцы 7-9)
**Цель:** Совместимость с 80% игр

- [ ] 6-button controller support
- [ ] Light gun (Lethal Enforcers)
- [ ] Mouse (Mickey Mania)
- [ ] Улучшенный FM (операторы, LFO, алгоритмы)
- [ ] Точные тайминги VDP (H-Blank, V-Blank)
- [ ] DMA controller
- [ ] Cheat system (Game Genie)
- [ ] Save states
- [ ] **Тест:** Streets of Rage 2, Mortal Kombat

### Фаза 4: Оптимизация (месяцы 10-12)
**Цель:** Full-speed на слабых системах

- [ ] **JIT-компилятор** 68000 → x86_64 (критично!)
- [ ] Multithreading (CPU/GPU/audio раздельно)
- [ ] OpenGL 3.3 рендерер (вместо software)
- [ ] Шейдеры: xBRZ upscaling, CRT-Lottes
- [ ] SIMD (SSE/AVX/NEON) для векторных операций
- [ ] Профилирование и оптимизация hot paths
- [ ] **Тест:** Virtua Racing, Star Wars Arcade (тяжёлые игры)

### Фаза 5: Современные контроллеры (месяцы 13-15)
**Цель:** DualSense — ваш киллер-фич!

- [ ] SDL2 GameController API (Xbox, Switch Pro)
- [ ] **DualSense через hidapi** (полная поддержка)
- [ ] Маппинг кнопок (авто + ручной)
- [ ] Профили per-game (JSON)
- [ ] Адаптивные триггеры (L2/R2 сопротивление)
- [ ] HD Haptic (направленная вибрация)
- [ ] Touchpad → дополнительные кнопки
- [ ] Gyro для motion-control (Space Harrier)
- [ ] RGB LED (смена цвета по событиям)
- [ ] **Тест:** Все игры с DualSense

### Фаза 6: Полировка (месяцы 16-18)
**Цель:** Продукт уровня "готово к релизу"

- [ ] UI на ImGui/Qt (настройки, ROM browser)
- [ ] Шейдеры: NTSC, scanlines, bloom
- [ ] Rewind feature (отматывание на 10 сек)
- [ ] Netplay (rollback netcode, опционально)
- [ ] Debugger (disassembler, memory viewer, breakpoints)
- [ ] Анализ совместимости (1000+ ROM тест)
- [ ] Документация, wiki
- [ ] **Релиз v1.0**

### Фаза 7: Экзотика (месяцы 19-24+)
**Цель:** Расширенные возможности

- [ ] Mega CD (CD-ROM, 6MB RAM, extra CPU)
- [ ] 32X (2x SH-2, extra GPU)
- [ ] Sega Meganet (модем)
- [ ] J-Cart (мультиплеер без адаптера)
- [ ] Прямой порт Saturn/DC (слишком сложно?)

## 🔬 Технические детали

### Cycle-accurate vs Fast

**Fast (текущий подход):**
- Достаточно для 95% игр
- Проще в реализации
- Быстрее

**Cycle-accurate:**
- 100% совместимость
- Требует точного эмулирования waitstates, bus contention
- Медленнее

**Рекомендация:** Начать с Fast, добавить cycle-accurate опционально.

### JIT-компиляция

**Почему JIT критичен:**
- Интерпретатор 68000: ~1-5 MIPS (медленно)
- JIT: 50-200 MIPS (full-speed)
- Ускорение: 10-50x

**Подход:**
1. **Basic blocks** — собираем последовательности инструкций
2. **Translation cache** — кэшируем скомпилированный код
3. **Recompilation** → x86_64/ARM64
4. **Invalidation** — при изменении памяти

**Библиотеки:**
- AsmJit (x86_64)
- DynASM (LuaJIT-style)
- Или свой простой JIT

### Тайминг и синхронизация

```
NTSC:
- 262 lines per frame
- 114 cycles per line (≈ 29868 cycles/frame)
- 60 Hz refresh

PAL:
- 313 lines per frame
- 114 cycles per line (≈ 35682 cycles/frame)
- 50 Hz refresh
```

**CPU синхронизация:**
- 68K и Z80 работают параллельно
- Shared memory (0xFF0000-0xFF3FFF)
- Waitstates при конфликтах

## 🎮 Ожидаемая совместимость

| Этап | Совместимость | Примеры игр |
|------|---------------|-------------|
| Фаза 1 (MVP) | 5% | Homebrew, демо |
| Фаза 2 | 40% | Sonic 1-2, Alex Kidd, Golden Axe |
| Фаза 3 | 70% | Streets of Rage 1-2, Mortal Kombat |
| Фаза 4 | 90% | Virtua Racing, Star Wars Arcade |
| Фаза 5+ | 95%+ | Почти все, кроме экзотики |

**Известные проблемные игры:**
- Virtua Fighter (требует точного timing)
- Daytona USA (сложный 3D)
- Mega CD/32X игры (доп. оборудование)

## 📚 Ресурсы для изучения

### Документация
- [Segaretro.org](https://segaretro.org) — огромная база знаний
- [MD Hardware Specs](https://segaretro.org/Mega_Drive)
- [68000 PRM](https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf)
- [VDP Documentation](https://www.omgwiki.com/doku.php/documents:assembly:megadrive:start)
- [YM2612 Manual](https://www.selapa.com/ym2612/)

### Код существующих эмуляторов
- **Genesis Plus GX** (C) — эталон чистоты
- **Kega Fusion** (C++) — эталон точности
- **Blastem** (C) — эталон производительности
- **MAME** — полная, но сложная

### Сообщества
- r/emulation на Reddit
- Discord: Emulation General
- Segaretro.org форум

## 🐛 Известные проблемы (на старте)

1. **CPU:** Реализована только малая часть инструкций 68000
2. **VDP:** Нет рендеринга спрайтов, только синий экран
3. **Audio:** FM — заглушка, PSG — базовый
4. **Input:** Только клавиатура, нет геймпадов
5. **Производительность:** Интерпретатор медленный

**Решение:** Постепенная реализация компонентов.

## 🎯 Критерии успеха MVP

Минимально рабочая версия должна:
- [ ] Загружать ROM (любой .bin/.md)
- [ ] Выводить изображение (хотя бы синий экран)
- [ ] Обрабатывать ввод с клавиатуры
- [ ] Издавать звук (хотя бы писк)
- [ ] Работать на 60 FPS

**Когда MVP готов — можно добавлять фичи.**

## 📈 Метрики прогресса

- **Количество строк кода:** ~15K (цель: 50K)
- **Покрытие инструкций 68000:** 10% → 100%
- **Совместимость игр:** 0% → 95%
- **FPS:** 0 → 60 (стабильно)
- **Задержка ввода:** < 2 frames

## 🎁 Бонус: Идеи для расширения

После релиза v1.0:
1. **Mega CD** — CD-ROM, 6MB RAM, extra CPU (SH-2)
2. **32X** — 2x SH-2, extra GPU (для 3D)
3. **Sega Saturn** — отдельный проект (слишком сложно)
4. **Dreamcast** — отдельный проект (PowerPC, PowerVR2)
5. **WebAssembly** — запуск в браузере
6. **Netplay** — многопользовательский режим

## 📞 Поддержка

Вопросы? Создайте Issue в репозитории.

---

**Удачи в разработке!** 🚀
