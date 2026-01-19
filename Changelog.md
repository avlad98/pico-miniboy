# Changelog - Pico-MiniBoy

## Current State
- **Performance**: 103 FPS rock-solid stable.
- **Hardware**: RP2040 (Pico) + 2.8" ILI9341 TFT SPI.
- **Clock**: 190MHz CPU, 95MHz SPI (Stable PIO 2-cycle).
- **Driver**: Custom PIO-based gapless SPI with DMA + Atomic Command Throttling + 1.3V VREG boost.
- **Color Modes**:
    - RGB565 (16-bit): Full color.
    - RGB444 (12-bit): 4096 colors, 1.5 bytes/pixel (Optimized block-filling).
    - RGB332 (8-bit): 256 colors, hardware-accelerated expansion via LUT.

---

## Commit History

### 1. Initial Commit (3f45483)
- **Message**: "Initial commit"
- project structure initialized.

### 2. Added 2.8'' TFT LCD demo (1a765eb)
- **Message**: "Added 2.8'' TFT LCD demo"
- Fundamental SPI communication with ILI9341.
- Basic display initialization and pixel drawing.

### 3. Digit Display Demo (7323457)
- **Message**: "Changed the LCD demo to display the digits from 0 to 9 in landscape mode"
- Implemented landscape mode support.
- Added demo showing digits 0-9.

### 4. Bouncing Ball & Overclocking (2d56718)
- **Message**: "Changed tft_lcd demo to bouncing ball, added FPS counter, overclocked CPU to 270Khz, SPI to 120Hz (no dma - 46 FPS)"
- Replaced digit demo with bouncing ball.
- Added FPS counter.
- Overclocked CPU to 270MHz.
- Initial SPI overclocking (120MHz theoretical via hardware SPI, but no DMA - 46 FPS).

### 5. DMA & 70MHz SPI (65eab5a)
- **Message**: "Overclocked CPU to 280Khz, SPI to 70MHz (with DMA -- 47 FPS)"
- Integrated DMA for display transfers.
- Overclocked CPU to 280MHz, SPI to 70MHz.
- Achieved 47 FPS.

### 6. Modular Architecture (8307f02, 3b3f81f)
- **Messages**: 
    - 8307f02: "Refactored code into modular architecture"
    - 3b3f81f: "Restructured project into modular library architecture"
- Refactored code into modular libraries (`lib/display`, `lib/graphics`, `lib/system_config`).
- Improved codebase structure for scalability.

### 7. Optimized Graphics Pipeline (80637fe)
- **Message**: "Optimized graphics pipeline to 64 FPS (100MHz SPI, 12mA drive, async DMA)"
- Increased SPI to 100MHz with 12mA drive strength and fast slew rate.
- Implemented asynchronous DMA transfers.
- Achieved 64 FPS.

### 8. Multi-format Support (905c636)
- **Message**: "Refactored graphics to support RGB444 (12-bit) and RGB565 (16-bit)"
- Refactored graphics library to support both RGB444 (12-bit) and RGB565 (16-bit).
- Support for different color resolutions to balance quality vs performance.

### 9. Parallel DMA Pipeline (8e9a9a0)
- **Message**: "Achieved 92 FPS with RGB444 (12-bit) Double Buffering and parallel DMA pipeline"
- Implemented double buffering for RGB444.
- Optimized DMA pipeline to allow parallel processing.
- Achieved 92 FPS.

### 10. Gapless PIO SPI (8a31a41)
- **Message**: "Achieved 98 FPS with PIO-based gapless SPI (90MHz, RGB444)"
- Replaced hardware SPI with a custom PIO-based gapless SPI driver.
- 2-cycle clock loop (SysClk / 2 SPI clock).
- Achieved 98 FPS at 90MHz SPI in RGB444 mode.

### 11. Record-Breaking 103 FPS Stability (83b68cb)
- **Message**: "Achieved 103 FPS with 95MHz SPI, Atomic Command Throttling, and 1.3V VREG boost"
- **Atomic Command Throttling**: Implemented safe-speed switching (10MHz for setup, 95MHz for data) to prevent display desync at high overclocks.
- **1.3V Booster**: Forced maximum core voltage for optimized GPIO switching timing.
- **High-Priority DMA**: Elevated display DMA priority to eliminate bus-starvation jitter.
- **Fast Word-Aligned Fills**: Rewrote RGB444 rectangle clearing to use 32-bit block writes.
- **Signal Integrity Barriers**: Added precision NOP delays to ensure signal settlement on the bus.
- **Result**: Surpassed previous 98 FPS record with better visual stability and no artifacts.

---

## Current Roadmap & Tasks

### High Priority: Performance Improvements
- [x] **Optimize RGB444 Filling**: Implemented fast-path 32-bit block writes for aligned rectangles.
- [x] **Optimize RGB332 Expansion**: Integrated LUT for zero-latency expansion.
- [x] **Command Throttling**: Implemented dynamic SPI frequency switching for stability.
- [ ] **DMA-Accelerated Clearing**: Use secondary DMA channel for background framebuffer clearing.
- [ ] **120FPS+ Hardware Validation**: Debug EXTREME profile on shorter signal paths.

### Feature Parity & Stability
- [ ] Ensure all performance improvements support RGB565 and RGB444 modes.
- [ ] Verify signal integrity at frequencies > 100MHz.
- [ ] Implement ST7735 driver (1.8" TFT).
- [ ] Add SD Card support and Button Matrix.
