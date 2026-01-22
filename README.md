# pico-miniboy

**Pico SDK demos for custom Gameboy-style handheld mini-computer**

## Project Purpose
- Test core subsystems for RP2040-based handheld emulation platform
- Verify 8/16-bit emulation performance limits (Pico + 2.8" SPI TFT)
- Modular C codebase using PIO-heavy drivers for custom OS/menu system
- Target: Doom, DOSBox-X, RetroArch cores, USB HID, modular peripherals

## Hardware Inventory

| Category | Components |
|----------|------------|
| **MCU** | Raspberry Pi Pico (RP2040), Tenstar RP2040 clone (16MB flash, USB-C) |
| **Display** | 2.8" TFT SPI 240x320 v1.1 (ILI9341), 1.8" TFT SPI 128x160 v1.1 (ST7735) |
| **Audio** | Phone buzzers, PC buzzers, vibration motor |
| **Input** | Tactile buttons, potentiometer, scrapped USB-A ports |
| **Debug** | 7-segment modules, 8x8 LED matrix, IR receiver |
| **Power** | Phone power bank (5V), 9V battery connector |
| **Tools** | 3D printer, soldering iron, resistor kit, Dupont wires |

## Development Environment
- Linux + VS Code with Raspberry Pi Pico extension
- Pico SDK 2.2.0 toolchain

## Current Status

| Demo | Status | Notes |
|------|--------|-------|
| Onboard LED | ✅ Working | 250ms blink during TFT ops |
| 2.8" TFT SPI | ✅ Working | ILI9341, >100 FPS (RGB444), Zero-Wait PIO Driver |
| 1.8" TFT SPI | ⏳ Planned | ST7735 driver |
| SD Card | ⏳ Planned | TFT module microSD slot |
| Buzzer PWM | ⏳ Planned | PWM + PIO DAC |
| Button matrix | ⏳ Planned | Debounced GPIO scan |
| USB Host HID | ⏳ Planned | Keyboard/mouse via TinyUSB |

## System Profiler
A modular profiling library (`lib/profiler`) is integrated to track real-time system performance.
- **Metrics**: FPS, CPU Usage (Core 0 & Core 1), RAM Usage (Heap), Flash Usage.
- **HUD**: Draws a non-intrusive bar at the top of the screen.
- **Usage**: Call `profiler_update()` and `profiler_draw()` in your main loop.

## Software Architecture
The project uses a layered architecture to separate hardware drivers from game logic:
1.  **Application Layer** (`demos/`): Implements `miniapp_desc_t` (Init/Update/Draw). Agnostic to hardware details.
2.  **Core Engine** (`lib/core`): Manages the main loop, system clocks, display initialization, and resource management.
3.  **Graphics Subsystem** (`lib/graphics`): Provides `surface_t`, drawing primitives, fonts, and multicore rendering services.
4.  **Hardware Drivers** (`lib/display`, `lib/system_config`): Zero-wait PIO SPI transport, DMA management, and RP2040 clock control.

## Optimization Roadmap & Experimentation Log

This section documents architectural experiments to prevent regression and guide future performance tuning.

### 1. General Architecture (All Modes)

| Idea | Expected Result | Actual Result | Conclusion |
|------|-----------------|---------------|------------|
| **Zero-Wait Hardware Sync** | Eliminate busy-wait CPU loops | **Success** (+15% FPS) | Using PIO `FDEBUG/TXSTALL` provides rock-solid sync without wasting cycles. **KEEP.** |
| **Strict Frame Barrier** | Prevent tearing by force-waiting | **Failed** (-30% FPS) | Do not force the CPU to wait for the *previous* frame to finish before clearing the *next* buffer. It destroys parallelism. |
| **Multicore Rendering** | Core 1 Draw / Core 0 Transfer | **Success** (2x Fill Rate) | implemented "Scanline-Interleaved" rendering. Core 1 clears Bottom Half (CPU), Core 0 clears Top Half (DMA). **KEEP.** |
| **Bus Priority Tuning** | Prevent DMA stutter | **Success** (Smoother) | Setting DMA Priority to HIGH in `bus_ctrl` helps maintain consistent frame times under load. **KEEP.** |

### 2. High-Frequency SPI (>100MHz)

| Idea | Expected Result | Actual Result | Conclusion |
|------|-----------------|---------------|------------|
| **110MHz SPI (2-Cycle)** | 119 FPS raw throughput | **Failed** (Diagonal Noise) | At 110MHz, the signal integrity on breadboards degrades, causing diagonal pixel artifacts. |
| **3-Cycle Harmonic Driver** | Stable 115 FPS via 3:1 Clock | **Failed** (Corrupt/Black) | The 3-cycle PIO logic (300MHz/100MHz) caused invalid state machine transitions or resonance issues on this specific chip. |
| **Clock Phase Inversion** | Fix diagonal noise by shifting SCK | **Failed** (No Change) | Inverting the clock phase at the GPIO level did not solve the reflections/noise at >100MHz. Signal integrity is the bottleneck. |
| **RGB444 Word-Aligned Fills**| Fast clearing | **Success** | Writing 32-bit blocks for 12-bit color (RGB444) works perfectly. **KEEP.** |

### 3. Current Stable Baseline
*   **CPU**: 190MHz
*   **SPI**: 95MHz (PERFORMANCE_HIGH)
*   **Driver**: 2-Cycle PIO, Mode 0 (Idle Low), 12mA Drive Strength
*   **FPS**: 103 FPS (Solid, Artifact-Free)

## Performance & Hardware Stress Test Matrix

This matrix tracks the stability and performance of the engine across all supported clock and color profiles.

| Profile | Format | FB | CPU/SPI (MHz) | FPS | C0/C1 Use | RAM | Status | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **STABLE** | RGB444 | 2 | 240 / 60 | 65 | 0% / 1% | 225K | ✅ Working | Baseline stability test |
| **BALANCED** | RGB444 | 2 | 270 / 67.5 | 73 | 0% / 1% | 225K | ✅ Working | Stable bump |
| **TURBO** | RGB444 | 2 | 160 / 80 | 86 | 0% / 2% | 225K | ✅ Working | |
| **HIGH** | RGB565 | 0 | 190 / 95 | 52 | 100% / 0% | 0K | ⚠️ Glitchy | Flickering ball/UI |
| **HIGH** | RGB565 | 1 | 190 / 95 | 58 | 100% / 0% | 150K | ✅ Working | Perfect visual fidelity |
| **HIGH** | RGB332 | 0 | 190 / 95 | - | - | - | ❌ Failing | Frozen at first frame |
| **HIGH** | RGB332 | 1 | 190 / 95 | 35 | 100% / 0% | 76K | ✅ Working | Perfect visual fidelity |
| **HIGH** | RGB332 | 2 | 190 / 95 | 35 | 100% / 0% | 151K | ✅ Working | Perfect visual fidelity |
| **HIGH** | RGB332 | 3 | 190 / 95 | 35 | 100% / 0% | 226K | ✅ Working | Perfect visual fidelity |
| **HIGH** | RGB444 | 0 | 190 / 95 | ~70 | - | 0K | ⚠️ Glitchy | Flickering, unintelligible UI |
| **HIGH** | RGB444 | 1 | 190 / 95 | 71 | 0% / 1% | 112K | ✅ Working | Perfect visual fidelity |
| **HIGH** | RGB444 | 2 | 190 / 95 | 102 | 0% / 2% | 225K | ✅ Working | **Best Performance** |
| **HIGH** | RGB444 | 3 | 190 / 95 | - | - | 266K+ | ❌ Failing | Static/stripes (RAM Overflow) |
| **MAX** | RGB444 | 2 | 220 / 110 | 119 | 0% / 2% | 225K | ⚠️ Glitchy | Cascading pixels / signal noise |
| **EXTREME** | RGB444 | 2 | 266 / 133 | 143 | 0% / 2% | 225K | ⚠️ Glitchy | Background turns black |

**Note**: Current failures (static/stripes) appear to be driver-related regressions.

## 2.8" TFT Wiring

| TFT Pin | Function | Pico GPIO | Pin # |
|---------|----------|-----------|-------|
| VCC | 3.3V | 3V3 | 36 |
| GND | Ground | GND | 38 |
| SCL | SCK | GP18 | 24 |
| SDA | MOSI | GP19 | 25 |
| RES | Reset | GP20 | 26 |
| DC/RS | D/C | GP21 | 27 |
| CS | Chip Sel | GP17 | 22 |
| LED/BL | Backlight | GP22 | 29 |

## Roadmap & Tasks
- **Bugfix**: Investigate and fix RGB565 and RGB332 modes (Regression).
- **Game Engine Core**: Parallel Sprite Batching & Dirty Rectangles
- **Input Driver**: Button Matrix / Debouncing / IO Expander
- **Audio Driver**: PWM / I2S Sound Engine
- **Storage**: SD Card support (SPI)
- **USB**: TinyUSB HOST HID (Keyboard/Mouse)
- **Display**: ST7735 (1.8") Support
