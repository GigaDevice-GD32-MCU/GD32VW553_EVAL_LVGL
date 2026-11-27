/*!
    \file    font_notosanscjksc_regular_14_b49ae336_fb3f.c
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

#include "lvgl.h"

LV_FONT_DECLARE(font_notosanscjksc_regular_14_b49ae336);

/*
 * Fallback wrapper for "font_notosanscjksc_regular_14_b49ae336": forwards every glyph lookup to the real font;
 * when a character is missing from it, re-queries the SAME font for codepoint
 * 0x3f (the project's configured Fallback Character) instead,
 * so unknown glyphs render as that character rather than the built-in placeholder box.
 *
 * Line-break control characters ('\n'/'\r') are excluded from the substitution:
 * the label draw loop walks the trailing '\n' of each line and relies on the normal
 * "no glyph" outcome to skip it; substituting would draw a stray fallback glyph per line break.
 *
 * LVGL 9: the bitmap callback receives the glyph descriptor, and the real font's callback
 * reads font->dsc through g_dsc->resolved_font. LVGL sets resolved_font to this wrapper
 * (whose dsc is NULL), so it is pointed at the real font for the duration of the call.
 */
static bool font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out,
                                           uint32_t letter, uint32_t letter_next)
{
    LV_UNUSED(font);
    if (font_notosanscjksc_regular_14_b49ae336.get_glyph_dsc(&font_notosanscjksc_regular_14_b49ae336, dsc_out, letter, letter_next)) {
        return true;
    }
    if (letter == '\n' || letter == '\r') {
        return false;
    }
    return font_notosanscjksc_regular_14_b49ae336.get_glyph_dsc(&font_notosanscjksc_regular_14_b49ae336, dsc_out, 63u, letter_next);
}

static const void * font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_bitmap(lv_font_glyph_dsc_t * g_dsc,
                                                      lv_draw_buf_t * draw_buf)
{
    const lv_font_t * wrapper = g_dsc->resolved_font;
    g_dsc->resolved_font = &font_notosanscjksc_regular_14_b49ae336;
    const void * bitmap = font_notosanscjksc_regular_14_b49ae336.get_glyph_bitmap(g_dsc, draw_buf);
    g_dsc->resolved_font = wrapper;
    return bitmap;
}

const lv_font_t font_notosanscjksc_regular_14_b49ae336_fb3f = {
    .get_glyph_dsc = font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_dsc,
    .get_glyph_bitmap = font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_bitmap,
    .line_height = 17,
    .base_line = 4,
    .subpx = 0,
    .underline_position = 0,
    .underline_thickness = 0,
    .static_bitmap = 0,
    .dsc = NULL,
    .fallback = NULL,
    .user_data = NULL,
};
