#ifndef RENDER_SERVICE_H
#define RENDER_SERVICE_H

#include "surface.h"
#include <stdint.h>

typedef enum {
  RENDER_CMD_IDLE,
  RENDER_CMD_CLEAR,
  RENDER_CMD_RECT,
  RENDER_CMD_CALLBACK,
  RENDER_CMD_EXIT
} render_command_t;

typedef struct {
  render_command_t type;
  surface_t *surface;
  uint32_t color32;
  uint16_t color16;
  uint32_t start_offset;
  uint32_t count;
  int x, y, w, h;
  void (*callback)(void *);
  void *callback_arg;
} render_job_t;

// Initialize the second core for rendering jobs
void render_service_init(void);

// Submit a job to the render service
void render_service_submit(const render_job_t *job);

// Wait for a job to complete
void render_service_wait(void);

// Get busy time from core 1
uint32_t render_service_get_busy_us(void);
void render_service_reset_stats(void);

#endif
