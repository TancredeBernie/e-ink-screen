// waveshare_epd.h
// Waveshare EPaper Display
// Implementation for Nordic nRF5x GFX Library
#include "nrf_lcd.h"
//to include trace system
#include "SEGGER_RTT.h"

// EPD1IN54 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define TERMINATE_FRAME_READ_WRITE                  0xFF

#define WSEPD_HEIGHT      200
#define WSEPD_WIDTH       200

ret_code_t wsepd154_init(void);
void wsepd154_uninit(void);
void wsepd154_pixel_draw(uint16_t x, uint16_t y, uint32_t color);
void wsepd154_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);
void wsepd154_display(void);
void wsepd154_rotation_set(nrf_lcd_rotation_t rotation);
void wsepd154_display_invert(bool invert);
void wsepd154_draw_monobmp(const uint8_t *image_buffer);
