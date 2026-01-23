#include "framebuffer.h"
#include "miniboy_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_BALLS 30
#define NUM_LINES 20
#define NUM_RECTS 20

typedef struct {
    float x, y;
    float vx, vy;
    int radius;
    uint16_t color;
} Ball;

typedef struct {
    int x1, y1, x2, y2;
    int dx, dy; // Movement for lines
    uint16_t color;
} Line;

typedef struct {
    float x, y;
    float vx, vy;
    int w, h;
    uint16_t color;
} RectObj;

static Ball balls[NUM_BALLS];
static RectObj rects[NUM_RECTS];

// Simple random generator for range
int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

uint16_t rand_color() {
    return rand() % 0xFFFF;
}

void game_init(void) {
    srand(12345); // Fixed seed for reproducibility

    // Init Balls
    for (int i = 0; i < NUM_BALLS; i++) {
        balls[i].x = rand_range(20, 300);
        balls[i].y = rand_range(20, 220);
        balls[i].vx = (rand() % 10 - 5);
        if (balls[i].vx == 0) balls[i].vx = 1;
        balls[i].vy = (rand() % 10 - 5);
        if (balls[i].vy == 0) balls[i].vy = 1;
        balls[i].radius = rand_range(5, 15);
        balls[i].color = rand_color();
    }

    // Init Rects
    for (int i = 0; i < NUM_RECTS; i++) {
        rects[i].x = rand_range(10, 280);
        rects[i].y = rand_range(10, 200);
        rects[i].w = rand_range(10, 50);
        rects[i].h = rand_range(10, 50);
        rects[i].vx = (rand() % 6 - 3);
        rects[i].vy = (rand() % 6 - 3);
        rects[i].color = rand_color();
    }
}

void game_update(uint32_t dt_us) {
    // Update Balls
    for (int i = 0; i < NUM_BALLS; i++) {
        balls[i].x += balls[i].vx;
        balls[i].y += balls[i].vy;

        // Bounce
        if (balls[i].x - balls[i].radius < 0) {
            balls[i].x = balls[i].radius;
            balls[i].vx = -balls[i].vx;
        } else if (balls[i].x + balls[i].radius >= 320) {
            balls[i].x = 320 - balls[i].radius - 1;
            balls[i].vx = -balls[i].vx;
        }

        if (balls[i].y - balls[i].radius < 0) {
            balls[i].y = balls[i].radius;
            balls[i].vy = -balls[i].vy;
        } else if (balls[i].y + balls[i].radius >= 240) {
            balls[i].y = 240 - balls[i].radius - 1;
            balls[i].vy = -balls[i].vy;
        }
    }

    // Update Rects
    for (int i = 0; i < NUM_RECTS; i++) {
        rects[i].x += rects[i].vx;
        rects[i].y += rects[i].vy;

        if (rects[i].x < 0 || rects[i].x + rects[i].w >= 320) rects[i].vx = -rects[i].vx;
        if (rects[i].y < 0 || rects[i].y + rects[i].h >= 240) rects[i].vy = -rects[i].vy;
    }
}

// Bresenham Line Algorithm
void draw_line_local(surface_t *surf, int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        draw_pixel(surf, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void game_draw(surface_t *surf) {
    // Clear screen (Dark Blue background)
    framebuffer_clear(0x000F);

    // Draw dynamic background lines connecting corners to balls
    for (int i = 0; i < NUM_LINES; i++) {
        if (i < NUM_BALLS) {
             draw_line_local(surf, 0, 0, (int)balls[i].x, (int)balls[i].y, 0x07E0); // Top-Left Green
             draw_line_local(surf, 319, 239, (int)balls[i].x, (int)balls[i].y, 0xF800); // Bottom-Right Red
        }
    }

    // Draw Rects
    for (int i = 0; i < NUM_RECTS; i++) {
        draw_rect(surf, (int)rects[i].x, (int)rects[i].y, rects[i].w, rects[i].h, rects[i].color);
    }

    // Draw Balls (Circles)
    for (int i = 0; i < NUM_BALLS; i++) {
        draw_circle(surf, (int)balls[i].x, (int)balls[i].y, balls[i].radius, balls[i].color);
    }
}

const miniapp_desc_t stress_test_app = {
    .name = "Stress Test",
    .init = game_init,
    .update = game_update,
    .draw = game_draw
};

int main() {
    engine_config_t cfg = {
        .width = 320,
        .height = 240,
        .pixel_format = PIXEL_FORMAT_RGB565,
        .performance_profile = PROFILE_HIGH,
        .buffer_count = 1
    };

    if (engine_init(&cfg)) {
        engine_run(&stress_test_app);
    }

    return 0;
}
