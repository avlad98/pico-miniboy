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

## Optimization Roadmap & Experimentation Log

This section documents architectural experiments to prevent regression and guide future performance tuning.

### 1. General Architecture (All Modes)

| Idea | Expected Result | Actual Result | Conclusion |
|------|-----------------|---------------|------------|
| **Zero-Wait Hardware Sync** | Eliminate busy-wait CPU loops | **Success** (+15% FPS) | Using PIO `FDEBUG/TXSTALL` provides rock-solid sync without wasting cycles. **KEEP.** |
| **Strict Frame Barrier** | Prevent tearing by force-waiting | **Failed** (-30% FPS) | Do not force the CPU to wait for the *previous* frame to finish before clearing the *next* buffer. It destroys parallelism. |
| **Multicore Rendering** | Core 1 Draw / Core 0 Transfer | **Unstable** | Introduced flickering and complexity without a clear gain over the optimized Single-Core pipeline yet. Parked for now. |
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

### Next
- PIO graphics DMA
- Multi-TFT support  
- Button matrix
- USB HID host
- Buzzer PWM
