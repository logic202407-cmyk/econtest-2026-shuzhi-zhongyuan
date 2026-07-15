/*********************************************************************************************************************
* MSPM0G3507 Opensource Library 即（MSPM0G3507 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* TianMengXing board adaptation: PB22 LED, PB21 key, UART0 PA10/PA11 via on-board CH340E.
* The original GPL-3.0 terms and copyright notices remain applicable.
********************************************************************************************************************/

#include "zf_common_headfile.h"

int main (void)
{
    uint8 key_latched = false;

    clock_init(SYSTEM_CLOCK_80M);                                               // 必须保留
    debug_init();                                                               // UART0 PA10/PA11，天猛星板载 CH340E
    tmx_board_init();

    printf("\r\nTianMengXing MSPM0G3507 + SeekFree Library V3.3.4 ready.\r\n");
    printf("LED: PB22 (active high), KEY: PB21 (active low).\r\n");

    while(true)
    {
        TMX_LED_TOGGLE();
        system_delay_ms(250);

        if(tmx_key_is_pressed())
        {
            if(!key_latched)
            {
                key_latched = true;
                printf("PB21 key pressed.\r\n");
            }
        }
        else
        {
            key_latched = false;
        }
    }
}
