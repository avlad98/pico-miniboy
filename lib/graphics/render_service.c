#include "render_service.h"
#include "pico/multicore.h"
#include "pico/time.h"

static volatile render_job_t current_job;
static volatile uint32_t core1_busy_us = 0;

static void core1_render_entry() {
  while (1) {
    uint32_t cmd = multicore_fifo_pop_blocking();
    uint32_t start_time = time_us_32();

    if (cmd == RENDER_CMD_EXIT)
      break;

    surface_t *surf = current_job.surface;
    if (!surf)
      goto done;

    if (cmd == RENDER_CMD_CLEAR) {
      uint8_t *fb_ptr = surf->pixels;
      if (surf->format == PIXEL_FORMAT_RGB444) {
        uint32_t w0, w1, w2;
        uint16_t c16 = current_job.color16;
        uint8_t r4 = ((c16 >> 11) & 0x1F) >> 1;
        uint8_t g4 = ((c16 >> 5) & 0x3F) >> 2;
        uint8_t b4 = (c16 & 0x1F) >> 1;
        uint8_t b0 = (r4 << 4) | g4;
        uint8_t b1 = (b4 << 4) | r4;
        uint8_t b2 = (g4 << 4) | b4;
        w0 = b0 | (b1 << 8) | (b2 << 16) | (b0 << 24);
        w1 = b1 | (b2 << 8) | (b0 << 16) | (b1 << 24);
        w2 = b2 | (b0 << 8) | (b1 << 16) | (b2 << 24);

        uint32_t *ptr32 = (uint32_t *)(fb_ptr + current_job.start_offset);
        uint32_t chunk_count = (current_job.count * 4) / 12;
        for (uint32_t i = 0; i < chunk_count; i++) {
          *ptr32++ = w0;
          *ptr32++ = w1;
          *ptr32++ = w2;
        }
      } else {
        uint32_t *ptr = (uint32_t *)(fb_ptr + current_job.start_offset);
        uint32_t *end = ptr + current_job.count;
        uint32_t val = current_job.color32;
        while (ptr < end)
          *ptr++ = val;
      }
    } else if (cmd == RENDER_CMD_CALLBACK) {
      if (current_job.callback) {
        current_job.callback(current_job.callback_arg);
      }
    }

  done:
    core1_busy_us += (time_us_32() - start_time);
    multicore_fifo_push_blocking(1);
  }
}

void render_service_init(void) { multicore_launch_core1(core1_render_entry); }

void render_service_submit(const render_job_t *job) {
  current_job = *job;
  multicore_fifo_push_blocking(job->type);
}

void render_service_wait(void) { multicore_fifo_pop_blocking(); }

uint32_t render_service_get_busy_us(void) { return core1_busy_us; }

void render_service_reset_stats(void) { core1_busy_us = 0; }
