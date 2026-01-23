# Changelog - Pico-MiniBoy

## Current State
- **Performance**: 103 FPS stable (Single-core RGB444).
- **Hardware**: RP2040 (Pico) + 2.8" ILI9341 TFT SPI.
- **Clock**: 190MHz CPU, 95MHz SPI (PERFORMANCE_HIGH).
- **Driver**: **Zero-Wait Architecture** - PIO FDEBUG/TXSTALL hardware sync replaces all CPU-wasteful NOP loops. 
- **Stability**: Resolved all command truncation and mirroring issues via precise 10MHz command throtling.
- **Color Modes**:
    - RGB565 (16-bit): Full color.
    - RGB444 (12-bit): 4096 colors, 1.5 bytes/pixel (Optimized block-filling).
    - RGB332 (8-bit): 256 colors, hardware-accelerated expansion via LUT.
- **Multicore Engine**: **Scanline-Interleaved** - Core 1 acts as a Render Slave, clearing the bottom half of the screen in parallel with Core 0. Double the fill-rate throughput.

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
- **Zero-Wait Hardware Sync**: Replaced NOP delays with PIO FDEBUG/TXSTALL monitoring for gapless performance.
- **Result**: Surpassed previous 98 FPS record with better visual stability and no artifacts.

### 12. Multicore & Profiler (Current)
- **Message**: "feat(core): Multicore Rendering & System Profiler (102 FPS)"
- **Multicore Engine**: Implemented Scanline-Interleaved clearing. Core 1 handles bottom half (CPU), Core 0 handles top half (DMA).
- **System Profiler**: Integrated `lib/profiler` HUD showing real-time CPU0/C1 usage, RAM, Flash, and FPS.
- **Font Engine**: Upgraded to full ASCII support with dynamic scaling.
- **Stability**: Fixed all multicore race conditions; Core 0 now sits at ~0% usage (fully offloaded), leaving 99% headroom for game logic.

### 13. Failed Optimization: Turbo DMA (32-bit Transfers)
- **Goal**: Improve bus throughput by using 32-bit DMA transfers (`DMA_SIZE_32`)
  and PIO Autopull 32, minimizing bus contention.
- **Attempt A (32-bit + BSWAP)**: Resulted in vertical stripes (alternating
  correct and garbage pixels), indicating a byte alignment mismatch between
  RP2040 32-bit LE read and Display 16-bit BE expectation.
- **Attempt B (RGB565 Aligned + Single Buffer Sync)**: Achieved stable 59 FPS
  but persistent vertical stripes remained even with aligned memory, proving
  that standard PIO shifting cannot correctly serialize 32-bit words for the
  ST7789 without custom byte-shuffling assembly.
- **Outcome**: Fully reverted to 8-bit DMA (Stable 102 FPS). The code for
  32-bit optimization was deemed too unstable/complex to keep in the codebase.

### 14. Level 1 Refactor: Transport/Panel Decoupling
- **Goal**: Enable multi-display support and separate transport logic from panel commands.
- **Display Transport**: Created a generic interface for PIO/SPI communication.
- **PIO Transport**: Encapsulated zero-wait hardware sync and DMA into a reusable module.
- **Panel Driver**: Refactored ILI9341 driver to use the transport interface, removing hardcoded PIO dependencies.
- **Result**: Codebase prepared for ST7735 and other display panels without duplicating driver logic.

### 15. Graphics Abstraction & Multicore Render Service
- **`surface_t` Interface**: Implemented a generic drawing surface abstraction.
- **`render_service`**: Extracted multicore rendering logic into a standalone service with job-offloading.
- **DMA Instrumentation**: Added wait-time tracking to framebuffer swap for performance profiling.
- **Memory Safety**: Implemented clean de-initialization for framebuffers and render services.

### 16. Telemetry HUD & System Instrumentation
- **Expanded HUD**: Upgraded to a 24px two-line overlay for high-density telemetry.
- **Clock Monitoring**: Integrated real-time CPU and SPI clock frequency display (MHz).
- **Auto-Detection**: HUD now automatically detects and displays resolution and pixel format.
- **System Profiling**: Integrated `mallinfo` for accurate heap utilization tracking.

### 18. Fix RGB332/RGB565 & Enable Direct Mode
- **Critical Fix**: Resolved regression where RGB332 and RGB565 modes were failing/frozen. They now work perfectly in single/double buffered configurations.
- **Buffer Management**: Fixed logic in `miniboy_engine` to correctly allocate buffers based on format size.
- **Direct Mode Support**: Fixed "Frozen/Crash" issues in Direct Mode (Buffer=0) by optimizing `font_draw_char` to use single-transaction buffering.
- **Direct Mode Limitations**: Flickering is expected in Direct Mode (no backbuffer), but it is now stable and functional for low-memory scenarios.
- **Telemetry**: Updated Profiler to correctly display "DIR" for Direct Mode and added Framebuffer count to performance tables.
- **Performance**:
    - RGB565 FB=1: Stable 58 FPS.
    - RGB332 FB=1: Stable 35 FPS.
    - RGB444 FB=2: Stable 102 FPS (Best Performance).

---

## Current Roadmap & Tasks

### High Priority: Performance Improvements
- [x] **Optimize RGB444 Filling**: Aligned 32-bit block writes.
- [x] **Optimize RGB332 Expansion**: Zero-latency LUT expansion.
- [x] **Command Throttling**: Dynamic SPI frequency switching.
- [x] **Zero-Wait Sync**: PIO hardware-level synchronization.
- [x] **Multicore Rendering**: Core 1 offloading (Complete).

### 19. Advanced Pipelined Flush & Core 1 Offloading
- **Message**: "feat(graphics): Core 1 Offloaded Flush & Pipelined RGB332 Optimization"
- **Optimized RGB332 Flush**: Replaced serial line-by-line flush with an async pipelined loop utilizing single-window mode.
- **Core 1 Offloading**: Implemented `RENDER_CMD_CALLBACK` to offload the RGB332 flush task entirely to Core 1.
- **Performance**: RGB332 1-FB performance now matches RGB444 (37 FPS in heavy stress test) while using only 76KB RAM.
- **Fix**: Resolved hanging issue in RGB444 mode by properly tracking swap type (DMA vs Core 1).
- **Stress Test**: Added "Chaos Mode" demo (30 balls, lines, rects) validating 99% Core 0 utilization.

### Feature Parity & Stability
- [x] **Bugfix**: Investigate and fix RGB565 and RGB332 regression.
- [x] **Decouple Transport/Panel**: generic transport interface (Complete).
- [ ] Implement ST7735 driver (1.8" TFT).
- [ ] Add Button Matrix Input Driver.
- [ ] Add SD Card support.
- [ ] Port Graphics engine to support sprite-based rendering.
