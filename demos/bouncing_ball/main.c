#include "pico/stdlib.h"
#include "system_config.h"
#include "display_driver.h"
#include "dma_spi.h"
#include "framebuffer.h"
#include "font.h"
#include <stdio.h>

int main() {
    // System configuration - LUDICROUS (266MHz CPU / 133MHz SPI)
    system_config_t sys_cfg = PERFORMANCE_LUDICROUS; 
    system_init(&sys_cfg);
    
    stdio_init_all();
    
    // Display configuration
    display_config_t disp_cfg = {
        .spi = spi0,
        .pin_cs = 17,
        .pin_sck = 18,
        .pin_mosi = 19,
        .pin_miso = 16,
        .pin_rst = 20,
        .pin_dc = 21,
        .pin_bl = 22,
        .width = 320,
        .height = 240,
        .format = PIXEL_FORMAT_RGB444 // 12-bit: ~97 FPS, 4096 colors
    };
    display_init(&disp_cfg, &sys_cfg);
    
    // Initialize DMA for framebuffer transfers
    dma_spi_init(display_get_spi());
    
    // Initialize framebuffer with chosen format
    if (!framebuffer_init(320, 240, disp_cfg.format)) {
        printf("ERROR: Failed to allocate framebuffer\n");
        return -1;
    }
    
    printf("CPU: %lu Hz, SPI: %lu Hz\n", 
           system_get_cpu_hz(), system_get_spi_hz());
    
    // Game state
    int circle_x = 160;
    int circle_y = 120;
    int circle_vx = 5;
    int circle_vy = 4;
    int circle_radius = 20;
    
    // FPS tracking
    uint32_t frame_count = 0;
    uint32_t fps_timer = time_us_32();
    int current_fps = 0;
    
    uint32_t clear_time = 0;
    uint32_t circle_time = 0;
    uint32_t swap_time = 0;
    
    while (1) {
        uint32_t t0 = time_us_32();
        
        // Clear screen (Red background for BGR displays)
        framebuffer_clear(0x0010);
        uint32_t t1 = time_us_32();
        
        // Update physics
        circle_x += circle_vx;
        circle_y += circle_vy;
        
        if (circle_x - circle_radius <= 0 || circle_x + circle_radius >= 320) {
            circle_vx = -circle_vx;
        }
        if (circle_y - circle_radius <= 0 || circle_y + circle_radius >= 240) {
            circle_vy = -circle_vy;
        }
        
        // Draw circle
        framebuffer_fill_circle(circle_x, circle_y, circle_radius, 0xFFE0);
        uint32_t t2 = time_us_32();
        
        // Draw FPS
        font_draw_number(10, 5, current_fps, 0xFFFF, 0x0000, &font_5x7);
        
        // Send to display (Asynchronous)
        // This returns immediately, allowing the next loop to clear/draw
        // while the SPI hardware is still sending the current buffer.
        framebuffer_swap_async();
        uint32_t t3 = time_us_32();
        
        frame_count++;
        clear_time = t1 - t0;
        circle_time = t2 - t1;
        swap_time = t3 - t2;
        
        // Calculate FPS
        uint32_t now = time_us_32();
        if (now - fps_timer >= 1000000) {
            current_fps = frame_count;
            printf("Clear: %lu us, Circle: %lu us, Swap: %lu us | FPS: %d\n", 
                   clear_time, circle_time, swap_time, current_fps);
            
            frame_count = 0;
            fps_timer = now;
        }
    }
    
    return 0;
}
