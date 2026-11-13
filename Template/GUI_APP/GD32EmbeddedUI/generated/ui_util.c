/*!
    \file    ui_util.c
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    All rights reserved.

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

#include <stdio.h>
#include "ui_util.h"

bool screen_is_loading(lv_obj_t *scr)
{
    /* lv_display_t fields are private in 9.x; prev screen is non-NULL while a screen-switch animation runs */
    lv_display_t *disp = lv_obj_get_display(scr);
    return lv_display_get_screen_prev(disp) != NULL;
}

void ui_style_init(lv_style_t *style)
{
    if (lv_style_is_empty(style))
        lv_style_init(style);
    else
        lv_style_reset(style);
}

lv_font_t *ui_font_init(char *path, int size)
{
#if LV_USE_FREETYPE
    return lv_freetype_font_create(path,
                                   LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                   size,
                                   LV_FREETYPE_FONT_STYLE_NORMAL);
#else
    (void)path;
    (void)size;
    return NULL;
#endif
}
