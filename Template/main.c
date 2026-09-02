/*
    \file    main.c
    \brief   LVGL demo on SPI LCD

    \version 2026-02-27, V1.6.0, demo for GD32VW55x
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32vw55x.h"
#include "systick.h"
#include <stdio.h>
#include "gd32vw553h_eval.h"
#include "lcd_driver.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_demos.h"
#include "ui_init.h"
#include "gui_app.h"

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    /* configure the systick (1ms tick) */
systick_config();

    /* configure USART0 for printf output */
    gd_eval_com_init(EVAL_COM0);

    lv_init();
    lcd_init();
    lv_port_disp_init();

    ui_init();
    custom_init();
    gd_eval_key_init(KEY_TAMPER_WAKEUP, KEY_MODE_EXTI);
    gui_app_button_init();

    while(1) {
        uint32_t idle = lv_timer_handler();

        gui_app_button_task();

        /* cap the sleep so the key is scanned often enough for debouncing */
        if(idle > 5U) {
            idle = 5U;
        }
        if(idle < 1U) {
            idle = 1U;
        }
        delay_1ms(idle);
    }
}




