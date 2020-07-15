// Waveshare EPaper Display
// Implementation for Nordic nRF5x GFX Library

#include "sdk_common.h"

#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

#include "waveshare154_epd.h"

//he one extra pin configured (SPI_MISO) doesn't get hooked up in hardware to anything,
//but is needed for the SPI module to work properly.
#define WSEPD_SPI_MISO  		27
#define WSEPD_SPI_MOSI  		31
#define WSEPD_SPI_CLK   		30
#define WSEPD_SPI_CS    		29
#define WSEPD_DC        		28
#define WSEPD_RST       		4
#define WSEPD_BUSY      		3
#define WSEPD_SPI_INSTANCE  0

const unsigned char lut_full_update[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const unsigned char lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(WSEPD_SPI_INSTANCE);
uint8_t screen_buffer[((WSEPD_WIDTH % 8 == 0)? (WSEPD_WIDTH / 8 ): (WSEPD_WIDTH / 8 + 1)) * WSEPD_HEIGHT];

static inline void spi_write(const void * data, size_t size)
{
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, data, size, NULL, 0));
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(WSEPD_DC);
    nrf_gpio_pin_clear(WSEPD_SPI_CS);
    spi_write(&c, sizeof(c));
    nrf_gpio_pin_set(WSEPD_SPI_CS);
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(WSEPD_DC);
    nrf_gpio_pin_clear(WSEPD_SPI_CS);
    spi_write(&c, sizeof(c));
    nrf_gpio_pin_set(WSEPD_SPI_CS);
}

static void set_cursor(uint16_t x, uint16_t y)
{
    write_command(SET_RAM_X_ADDRESS_COUNTER);
    write_data((x >> 3) & 0xff);

    write_command(SET_RAM_Y_ADDRESS_COUNTER);
    write_data(y & 0xFF);
    write_data((y >> 8) & 0xFF);
}

static void set_addr_window(uint16_t x_0, uint16_t y_0, uint16_t x_1, uint16_t y_1)
{
    ASSERT(x_0 < x_1);
    ASSERT(y_0 < y_1);

    write_command(SET_RAM_X_ADDRESS_START_END_POSITION);
    write_data((x_0 >> 3) & 0xff);
    write_data((x_1 >> 3) & 0xff);
    write_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
    write_data(y_0 & 0xff);
    write_data((y_0 >> 8) & 0xff);
    write_data(y_1 & 0xff);
    write_data((y_1 >> 8) & 0xff);
}

static void wait_until_idle(void)
{
    SEGGER_RTT_WriteString(0,"epd busy\r\n");
    while(nrf_gpio_pin_read(WSEPD_BUSY) == 0) {      //LOW: idle, HIGH: busy
        nrf_delay_ms(100);
    }
		SEGGER_RTT_WriteString(0,"epd busy release\r\n");
}

static void turn_on_display(void)
{
    write_command(DISPLAY_UPDATE_CONTROL_2);
    write_data(0xc4);
    write_command(MASTER_ACTIVATION);
    write_command(TERMINATE_FRAME_READ_WRITE);

    wait_until_idle();
}

static void wsepd_reset(void)
{
    nrf_gpio_pin_set(WSEPD_RST);
    nrf_delay_ms(200);
    nrf_gpio_pin_clear(WSEPD_RST);
    nrf_delay_ms(200);
    nrf_gpio_pin_set(WSEPD_RST);
    nrf_delay_ms(200);
}

static void command_list(void)
{
    wsepd_reset();

    write_command(DRIVER_OUTPUT_CONTROL);
    write_data((WSEPD_HEIGHT - 1) & 0xFF);
    write_data(((WSEPD_HEIGHT - 1) >> 8) & 0xFF);
    write_data(0x00);
    write_command(BOOSTER_SOFT_START_CONTROL);
    write_data(0xD7);
    write_data(0xD6);
    write_data(0x9D);
    write_command(WRITE_VCOM_REGISTER);
    write_data(0xA8);
    write_command(SET_DUMMY_LINE_PERIOD);
    write_data(0x1A);
    write_command(SET_GATE_TIME);
    write_data(0x08);
    write_command(DATA_ENTRY_MODE_SETTING);
    write_data(0x03);

    write_command(WRITE_LUT_REGISTER);
    for(uint8_t i = 0; i < 30; i++)
    {
        // TODO: allow partial update mode
        write_data(lut_full_update[i]);
    }
}

static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(WSEPD_DC);
    nrf_gpio_cfg_output(WSEPD_RST);
    nrf_gpio_cfg_output(WSEPD_SPI_CS);

    nrf_gpio_cfg_input(WSEPD_BUSY, NRF_GPIO_PIN_NOPULL);

    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

    spi_config.sck_pin  = WSEPD_SPI_CLK;
    spi_config.miso_pin = WSEPD_SPI_MISO;
    spi_config.mosi_pin = WSEPD_SPI_MOSI;

    err_code = nrf_drv_spi_init(&spi, &spi_config, NULL, NULL);
    return err_code;
}

ret_code_t wsepd154_init(void)
{
    ret_code_t err_code;

    err_code = hardware_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    command_list();
    // Clear screen buffer to white
    memset(screen_buffer, 0xff, sizeof(screen_buffer));
    return 0;
}

void wsepd154_uninit(void)
{
    //ASSERT(wsepd154_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    write_command(DEEP_SLEEP_MODE);
    write_data(0x01);
    nrf_drv_spi_uninit(&spi);
}


void wsepd154_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    uint16_t x_, y_;
 
    if(x > WSEPD_WIDTH || y > WSEPD_HEIGHT){
				SEGGER_RTT_WriteString(0,"Exceeding display boundaries 1\r\n");
        return;
    }
		//BCH to test for the moment
		x_ = x;
		y_ = y;
    /*switch(wsepd154_cb.rotation) 
    {
        case NRF_LCD_ROTATE_90:
            x_ = WSEPD_WIDTH - y - 1;
            y_ = x;
            break;
        case NRF_LCD_ROTATE_180:
            x_ = WSEPD_WIDTH - x - 1;
            y_ = WSEPD_HEIGHT - y - 1;
            break;
        case NRF_LCD_ROTATE_270:
            x_ = y;
            y_ = WSEPD_HEIGHT - x - 1;
            break;
        default:
            x_ = x;
            y_ = y;
            break;
    }*/
    if(x_ > WSEPD_WIDTH || y_ > WSEPD_HEIGHT){
        SEGGER_RTT_WriteString(0,"Exceeding display boundaries 2\r\n");
        return;
    }
    uint16_t addr = x_ / 8 + y_ * ((WSEPD_WIDTH % 8) == 0 ? WSEPD_WIDTH / 8 : WSEPD_WIDTH / 8 + 1);
    uint8_t rdata = screen_buffer[addr];
    if(color == 0)
        screen_buffer[addr] = rdata & ~(0x80 >> (x_ % 8));
    else
        screen_buffer[addr] = rdata | (0x80 >> (x_ % 8));
}

void wsepd154_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    uint16_t x_, y_;
 
    // TODO: This could be done about 8x faster with cheeky bitfield manipulations
    //     but it's really difficult to get it all perfect given the 6 conditions
    for(y_ = y; y_ < (y + height); y_ ++)
    {
        for(x_ = x; x_ < (x + width); x_ ++)
        {
            wsepd154_pixel_draw(x_, y_, color);
        }
    }
}

void wsepd154_display(void)
{
    uint16_t width;
    uint32_t addr = 0;
    uint16_t i, j;

    width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);
    set_addr_window(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT);
    for(i = 0; i < WSEPD_HEIGHT; i++)
    {
        set_cursor(0, i);
        write_command(WRITE_RAM);
        for(j = 0; j < width; j++)
        {
            addr = j + i * width;
            write_data(screen_buffer[addr]);
        }
    }
    turn_on_display();
}

void wsepd154_rotation_set(nrf_lcd_rotation_t rotation)
{
    // dummy function - actions are taken in the drawing primitives
}

void wsepd154_display_invert(bool invert)
{
    // dummy function - doesn't have feature built in
}

// Can only draw an image the size of screen or larger
void wsepd154_draw_monobmp(const uint8_t *image_buffer)
{
    uint16_t x, y;
    uint32_t addr = 0;
    uint8_t byte_width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);

    for (y = 0; y < WSEPD_HEIGHT; y++) {
        for (x = 0; x < byte_width; x++) 
        {//8 pixel =  1 byte
            addr = x + y * byte_width;
            screen_buffer[addr] = (uint8_t)image_buffer[addr];
        }
    }
}


