#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Match tjpgd example: EXAMPLE_LCD_H_RES=320, V_RES=240 (landscape). */
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

/* Convenience RGB565 colors, already byte-swapped for the panel's big-endian
 * pixel format (the SPI bus ships color bytes high-first). */
#define DISPLAY_BLACK 0x0000
#define DISPLAY_WHITE 0xFFFF
#define DISPLAY_RED 0x00F8
#define DISPLAY_GREEN 0xE007
#define DISPLAY_BLUE 0x1F00

  /* Pack three 8-bit channels into an RGB565 in panel byte order. */
  static inline uint16_t display_rgb(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t v = (uint16_t)((r & 0xF8) << 8) | (uint16_t)((g & 0xFC) << 3) | (uint16_t)((b & 0xF8) >> 3);
    return (uint16_t)((v >> 8) | (v << 8));
  }

  /* Bring up SPI2 + ST7789, run software reset, turn the panel on.
 * Idempotent: calling twice returns ESP_OK without touching hardware. */
  esp_err_t display_init(void);
  /* Fill the entire visible area with a single RGB565 color (panel byte order). */
  esp_err_t display_fill(uint16_t color);
  /* Blit a pixel buffer. (x, y) is the top-left corner; w*h pixels in row-major
 * RGB565 (panel byte order). x+w must be <= DISPLAY_WIDTH after rotation;
 * same for y+h. */
  esp_err_t display_draw(int x, int y, int w, int h, const uint16_t *pixels);
  /* One-shot R/G/B/white sweep, ~500 ms each. Useful for confirming wiring. */
  void display_self_test(void);

  esp_err_t display_image(uint8_t *map);
#ifdef __cplusplus
}
#endif
