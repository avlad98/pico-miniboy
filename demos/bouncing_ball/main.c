#include "framebuffer.h" // For drawing primitives
#include "miniboy_engine.h"
#include <stdio.h>

// Game state
static int circle_x = 160;
static int circle_y = 120;
static int circle_vx = 5;
static int circle_vy = 4;
static int circle_radius = 20;

void game_init(void) {
  // Reset state if needed
  circle_x = 160;
  circle_y = 120;
  circle_vx = 5;
  circle_vy = 4;
}

void game_update(uint32_t dt_us) {
  // Update physics
  circle_x += circle_vx;
  circle_y += circle_vy;

  if (circle_x - circle_radius <= 0 || circle_x + circle_radius >= 320) {
    circle_vx = -circle_vx;
  }
  if (circle_y - circle_radius <= 0 || circle_y + circle_radius >= 240) {
    circle_vy = -circle_vy;
  }
}

void game_draw(surface_t *surf) {
  // Clear screen (Red background for BGR displays)
  framebuffer_clear(0x0010);

  // Draw circle
  draw_circle(surf, circle_x, circle_y, circle_radius, 0xFFE0);
}

const miniapp_desc_t bouncing_ball_app = {.name = "Bouncing Ball",
                                          .init = game_init,
                                          .update = game_update,
                                          .draw = game_draw};

int main() {
  engine_config_t cfg = {
      .width = 320,
      .height = 240,
      .pixel_format = PIXEL_FORMAT_RGB444,
      .performance_profile = PROFILE_HIGH,
      .buffer_count = 2
  };

  if (engine_init(&cfg)) {
    engine_run(&bouncing_ball_app);
  }

  return 0;
}
