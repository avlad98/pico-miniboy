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
| 2.8" TFT SPI | ✅ Working | ILI9341, 10MHz SPI, RGB565 color fill |
| 1.8" TFT SPI | ⏳ Planned | ST7735 driver |
| SD Card | ⏳ Planned | TFT module microSD slot |
| Buzzer PWM | ⏳ Planned | PWM + PIO DAC |
| Button matrix | ⏳ Planned | Debounced GPIO scan |
| USB Host HID | ⏳ Planned | Keyboard/mouse via TinyUSB |

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
