/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 * Copyright (c) 2026, GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* This file refers the RISC-V standard, some adjustments are made according to GigaDevice chips */

#include "gd32vw55x.h"
#include <stdio.h>
#include <stdarg.h>

int __wrap_printf(const char* fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    for(int i = 0; i < len && i < (int)sizeof(buf); i++) {
        while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
        usart_data_transmit(USART0, (uint8_t)buf[i]);
    }
    return len;
}

