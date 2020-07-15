// Waveshare EPaper Display
// Implementation for Nordic nRF5x GFX Library

#include "sdk_common.h"

#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

#include "waveshare154_V2_epd.h"

//Driver functions

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(WSEPD_SPI_INSTANCE);
uint8_t screen_buffer[((WSEPD_WIDTH % 8 == 0)? (WSEPD_WIDTH / 8 ): (WSEPD_WIDTH / 8 + 1)) * WSEPD_HEIGHT];

static inline void spi_write(const void * data, size_t size)
{
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, data, size, NULL, 0));
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(SPI_EPD_DC_PIN);
    nrf_gpio_pin_clear(SPI_EPD_CS_PIN);
    spi_write(&c, sizeof(c));
    nrf_gpio_pin_set(SPI_EPD_CS_PIN);
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(SPI_EPD_DC_PIN);
    nrf_gpio_pin_clear(SPI_EPD_CS_PIN);
    spi_write(&c, sizeof(c));
    nrf_gpio_pin_set(SPI_EPD_CS_PIN);
}

static void wait_until_idle(void)
{
    SEGGER_RTT_WriteString(0,"epd busy\r\n");
    while(nrf_gpio_pin_read(SPI_EPD_BUSY_PIN) == 1) {      //LOW: idle, HIGH: busy
        nrf_delay_ms(100);
    }
		SEGGER_RTT_WriteString(0,"epd busy release\r\n");
}

static void turn_on_display(void)
{
    write_command(DISPLAY_UPDATE_CONTROL_2);
    write_data(0xF7);
    write_command(MASTER_ACTIVATION);

    wait_until_idle();
}

static void turn_on_display_part(void)
{
    write_command(DISPLAY_UPDATE_CONTROL_2);
    write_data(0xFF);
    write_command(MASTER_ACTIVATION);

    wait_until_idle();
}

static void wsepd_reset(void)
{
    nrf_gpio_pin_set(SPI_EPD_RST_PIN);
    nrf_delay_ms(200);
    nrf_gpio_pin_clear(SPI_EPD_RST_PIN);
    nrf_delay_ms(200);
    nrf_gpio_pin_set(SPI_EPD_RST_PIN);
    nrf_delay_ms(200);
}

static void command_list(void)
{
    wsepd_reset();
	
		wait_until_idle();
		write_command(SW_RESET);
		wait_until_idle();

    write_command(DRIVER_OUTPUT_CONTROL);
    write_data((WSEPD_HEIGHT - 1) & 0xFF);//0xC7
    write_data(((WSEPD_HEIGHT - 1) >> 8) & 0xFF);//0x00
    write_data(DRIVER_OUTPUT_CONTROL);
	
		//data entry mode
		write_command(DATA_ENTRY_MODE_SETTING);
		write_data(DRIVER_OUTPUT_CONTROL);
	
		//set RAM-X address start/end position
		write_command(SET_RAM_X_ADDRESS_START_END_POSITION);
		write_data(0x00);
    write_data(0x18);    //0x0C-->(18+1)*8=200
	
		//set RAM-Y address start/end position
		write_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
		write_data((WSEPD_HEIGHT - 1) & 0xFF);//0xC7
		write_data(0x00);
		write_data(0x00);
		write_data(0x00);
	
		write_command(BORDER_WAVEFORM_CONTROL);
		write_data(0x01);
		write_command(0x18);
		write_data(0x80);
		
		write_command(DISPLAY_UPDATE_CONTROL_2); // //Load Temperature and waveform setting.
    write_data(0xB1);
    write_command(MASTER_ACTIVATION);
		
    write_command(SET_RAM_X_ADDRESS_COUNTER);   // set RAM x address count to 0;
    write_data(0x00);
    write_command(SET_RAM_Y_ADDRESS_COUNTER);   // set RAM y address count to 0X199;
    write_data(0xC7);
    write_data(0x00);
    wait_until_idle();
}

static void clear() {
		/*uint16_t Width, Height;
    Width = (WSEPD_WIDTH % 8 == 0)? (WSEPD_WIDTH / 8 ): (WSEPD_WIDTH / 8 + 1);
    Height = WSEPD_HEIGHT;
	
		write_command(WRITE_RAM);
		// Clear screen buffer to white
		for (uint16_t j = 0; j < Height; j++) {
					for (uint16_t i = 0; i < Width; i++) {
							write_data(0XFF);
					}
			}*/
		uint16_t Width, Height;
    Width = (WSEPD_WIDTH % 8 == 0)? (WSEPD_WIDTH / 8 ): (WSEPD_WIDTH / 8 + 1);
    Height = WSEPD_HEIGHT;

    uint32_t Addr = 0;
    write_command(WRITE_RAM);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            Addr = i + j * Width;
            write_data(screen_buffer[Addr]);
        }
    }
		
		//turn_on_display();
}

static void sleep(void) {
		write_command(DEEP_SLEEP_MODE); //enter deep sleep
    write_data(0x01);
    nrf_delay_ms(100);
}

static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(SPI_EPD_DC_PIN);
    nrf_gpio_cfg_output(SPI_EPD_RST_PIN);
    nrf_gpio_cfg_output(SPI_EPD_CS_PIN);

    nrf_gpio_cfg_input(SPI_EPD_BUSY_PIN, NRF_GPIO_PIN_NOPULL);

    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

    spi_config.sck_pin  = SPI_SCK_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;

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
		
		memset(screen_buffer, 0xff, sizeof(screen_buffer));

    command_list();
		
		//clear();
    
    return 0;
}

void wsepd154_uninit(void)
{
    //ASSERT(wsepd154_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    sleep();
    nrf_drv_spi_uninit(&spi);
}


void wsepd154_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    uint16_t x_, y_;
 
    if(x > WSEPD_WIDTH || y > WSEPD_HEIGHT){
				SEGGER_RTT_WriteString(0,"Exceeding display boundaries 1\r\n");
        return;
    }

    switch(rotation) 
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
    }
		
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
    /*uint16_t Width, Height;
    Width = (WSEPD_WIDTH % 8 == 0)? (WSEPD_WIDTH / 8 ): (WSEPD_WIDTH / 8 + 1);
    Height = WSEPD_HEIGHT;

    uint32_t Addr = 0;
    write_command(WRITE_RAM);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            Addr = i + j * Width;
            write_data(screen_buffer[Addr]);
        }
    }*/
		clear();
    turn_on_display();
}

void wsepd154_rotation_set(nrf_lcd_rotation_t rt)
{
    rotation = rt;
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


