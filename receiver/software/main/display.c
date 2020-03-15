
#include "display.h"
#include "driver/spi.h"
#include "driver/gpio.h"
#include "esp8266/spi_struct.h"

DISPLAY_HandleTypeDef hdisplay;

void Display_update(DISPLAY_HandleTypeDef *hdisplay) {
    uint32_t buf = 0;
    if (hdisplay->state == DISPLAY_STATE_ON) {
      // Write 8-bit data to the shift register.
      buf = hdisplay->pixels << 16;
    }
    spi_trans_t trans = {0};
    trans.mosi = &buf;
    trans.bits.mosi = 16;
    spi_trans(HSPI_HOST, trans);
}

void Display_on(DISPLAY_HandleTypeDef *hdisplay) {
    DISPLAY_StateTypeDef tmp = hdisplay->state;
    hdisplay->state = DISPLAY_STATE_ON;
    if (tmp == DISPLAY_STATE_OFF) {
        Display_update(hdisplay);
    }
}

void Display_off(DISPLAY_HandleTypeDef *hdisplay) {
    DISPLAY_StateTypeDef tmp = hdisplay->state;
    hdisplay->state = DISPLAY_STATE_OFF;
    if (tmp == DISPLAY_STATE_ON) {
        Display_update(hdisplay);
    }
}

static void setupGPIO() {
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = (1ULL << DISPLAY_PIN_BLUE);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void setupSPI() {
    spi_config_t spi_config;
    // Load default interface parameters
    // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    spi_config.mode = SPI_MASTER_MODE;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_10MHz_DIV;
    // Cancel MISO
    spi_config.interface.miso_en = 0;
    spi_init(HSPI_HOST, &spi_config);
}

void Display_init() {
    setupGPIO();
    setupSPI();
}
