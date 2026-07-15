/*********************************************************************************************************************
* TianMengXing MSPM0G3507 board adaptation for SeekFree MSPM0G3507 Library V3.3.4
*
* This file adds board-level definitions only. The underlying SeekFree library remains GPL-3.0 and its original
* copyright notices must be preserved.
********************************************************************************************************************/

#ifndef _zf_common_board_tianmengxing_h_
#define _zf_common_board_tianmengxing_h_

// Board resources verified from the LCSC TianMengXing MSPM0G3507 schematic.
#define TMX_LED_PIN                    (B22)      // User LED, active high
#define TMX_KEY_PIN                    (B21)      // User key, active low, connect to GND when pressed

#define TMX_DEBUG_UART_INDEX           (UART_0)
#define TMX_DEBUG_UART_TX_PIN          (UART0_TX_A10)
#define TMX_DEBUG_UART_RX_PIN          (UART0_RX_A11)

#define TMX_FLASH_CS_PIN               (B6)       // On-board SPI Flash CS#
#define TMX_FLASH_MISO_PIN             (B7)
#define TMX_FLASH_MOSI_PIN             (B8)
#define TMX_FLASH_SCK_PIN              (B9)

#define TMX_BSL_PIN                    (A18)
#define TMX_SWDIO_PIN                  (A19)
#define TMX_SWCLK_PIN                  (A20)

#define TMX_LED_ON()                   gpio_set_level(TMX_LED_PIN, GPIO_HIGH)
#define TMX_LED_OFF()                  gpio_set_level(TMX_LED_PIN, GPIO_LOW)
#define TMX_LED_TOGGLE()               gpio_toggle_level(TMX_LED_PIN)

// Initialize only board-fixed resources. External peripherals should be initialized by the application.
static inline void tmx_board_init (void)
{
    // Keep the on-board SPI Flash deselected. PB7/PB8/PB9 remain available for SPI1 when needed.
    gpio_init(TMX_FLASH_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);

    // User LED is connected PB22 -> resistor -> LED -> GND, so high level turns it on.
    gpio_init(TMX_LED_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);

    // User key shorts PB21 to GND when pressed.
    gpio_init(TMX_KEY_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
}

static inline uint8 tmx_key_is_pressed (void)
{
    return (GPIO_LOW == gpio_get_level(TMX_KEY_PIN));
}

#endif
