#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "m_powered_320x240.h"

/* GPIO numbers per the dcsgonebadge schematic (`schematic/inoutput.kicad_sch`
 * + the MCU sheet) — these are the *datasheet GPIO numbers* of the ESP32-C6,
 * which is what ESP-IDF expects, NOT the chip's package pin numbers:
 *   SPI_SCK  net → GPIO 5    (J3 pin 6)
 *   SPI_CS1  net → GPIO 4    (J3 pin 5)
 *   TFT_DC   net → GPIO 2    (J3 pin 7)
 *   SPI_SDO  net → GPIO 11   (J3 pin 2)  ← the panel's data input
 *   TFT_RST  net → GPIO 0    (× in schematic = not actually routed to FPC,
 *                             panel reset is driven by the on-board RC delay)
 * The badge dev's tjpgd config (6/5/4/3) appears to target a different board
 * revision; on this schematic GPIO 6 is the ALSense (light sensor) input
 * and GPIO 3 isn't routed at all. */
#define PIN_SCK 5
#define PIN_MOSI 11
#define PIN_CS 4
#define PIN_DC 3
#define PIN_RST -1

#define LCD_HOST SPI2_HOST
/* tjpgd example: EXAMPLE_LCD_PIXEL_CLOCK_HZ = 20 MHz. */
#define PCLK_HZ (20 * 1000 * 1000)

static const char *TAG = "display";
static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_panel_handle_t s_panel;
static bool s_inited;

#define FILL_ROWS 16
static uint16_t *s_line_buf;
static size_t s_line_buf_count;

esp_err_t display_init(void) {
  if (s_inited) return ESP_OK;
  ESP_LOGI(TAG, "init: SCK=%d MOSI=%d CS=%d DC=%d RST=%d", PIN_SCK, PIN_MOSI, PIN_CS, PIN_DC, PIN_RST);
  /* Allocate the line buffer in DMA-capable internal RAM with proper
     * alignment, exactly like the tjpgd example does for its s_lines[]. */
  s_line_buf_count = (size_t)DISPLAY_WIDTH * (size_t)FILL_ROWS;
  s_line_buf = heap_caps_malloc(s_line_buf_count * sizeof(uint16_t), MALLOC_CAP_DMA);
  if (!s_line_buf) {
    ESP_LOGE(TAG, "out of DMA memory for line buffer");
    return ESP_ERR_NO_MEM;
  }
  spi_bus_config_t bus = {
    .sclk_io_num = PIN_SCK,
    .mosi_io_num = PIN_MOSI,
    .miso_io_num = -1,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = DISPLAY_WIDTH * FILL_ROWS * 2 + 16,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));
  esp_lcd_panel_io_spi_config_t io_cfg = {
    .dc_gpio_num = PIN_DC,
    .cs_gpio_num = PIN_CS,
    .pclk_hz = PCLK_HZ,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .spi_mode = 0,
    .trans_queue_depth = 10,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
    (esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &s_io));
  esp_lcd_panel_dev_config_t panel_cfg = {
    .reset_gpio_num = PIN_RST,
    .rgb_endian = LCD_RGB_ENDIAN_RGB,
    .bits_per_pixel = 16,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io, &panel_cfg, &s_panel));
  /* Verbatim init order from the tjpgd example (lines 144-154) PLUS the
     * diff's mirror change (line 174). The original toggles is_rotated each
     * frame; the diff removes the toggle and locks mirror to (false, true). */
  ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, false, true));
  s_inited = true;
  ESP_LOGI(
    TAG, "init OK: %dx%d @ %d MHz",
    DISPLAY_WIDTH, DISPLAY_HEIGHT, PCLK_HZ / 1000000);
  return ESP_OK;
}

esp_err_t display_draw(int x, int y, int w, int h, const uint16_t *pixels) {
  if (!s_inited) return ESP_ERR_INVALID_STATE;
  if (!pixels) return ESP_ERR_INVALID_ARG;
  return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + h, pixels);
}

esp_err_t display_fill(uint16_t color) {
  if (!s_inited) return ESP_ERR_INVALID_STATE;
  for (size_t i = 0; i < s_line_buf_count; i++) {
    s_line_buf[i] = color;
  }
  for (int y = 0; y < DISPLAY_HEIGHT; y += FILL_ROWS) {
    int rows = (y + FILL_ROWS <= DISPLAY_HEIGHT) ? FILL_ROWS : (DISPLAY_HEIGHT - y);
    esp_err_t err = esp_lcd_panel_draw_bitmap(
      s_panel,
      0, y, DISPLAY_WIDTH, y + rows,
      s_line_buf);
    if (err != ESP_OK) return err;
  }
  return ESP_OK;
}

esp_err_t display_image(uint8_t *map) {
  if (!s_inited) return ESP_ERR_INVALID_STATE;
  esp_err_t err = esp_lcd_panel_draw_bitmap(
    s_panel,
    0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
    (uint16_t*)map);
    if (err != ESP_OK) return err;
  return ESP_OK;
}

void display_self_test(void) {
  if (!s_inited) return;
//   static const uint16_t seq[] = {
//     DISPLAY_RED, DISPLAY_GREEN,
//     DISPLAY_BLUE, DISPLAY_WHITE
//   };
  display_fill(DISPLAY_GREEN);
  vTaskDelay(pdMS_TO_TICKS(800));
  display_image(mpowered_map);
}
