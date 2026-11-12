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
 * Line-break control characters ('\n'/'\r') are deliberately excluded from the
 * fallback substitution: LVGL's label/dropdown draw loop includes the trailing
 * '\n' of each line in the glyph range it walks (see lv_draw_label.c), relying on
 * the normal "no glyph for this codepoint" outcome to silently skip it. If the
 * fallback were applied here too, every line break would render as a visible
 * fallback-character glyph (e.g. a stray '?' after every dropdown option).
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

static const uint8_t * font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_bitmap(const lv_font_t * font, uint32_t letter)
{
    LV_UNUSED(font);
    if (letter == '\n' || letter == '\r') {
        return NULL;
    }
    lv_font_glyph_dsc_t dsc;
    uint32_t resolved_letter =
        font_notosanscjksc_regular_14_b49ae336.get_glyph_dsc(&font_notosanscjksc_regular_14_b49ae336, &dsc, letter, 0) ? letter : 63u;
    return font_notosanscjksc_regular_14_b49ae336.get_glyph_bitmap(&font_notosanscjksc_regular_14_b49ae336, resolved_letter);
}

const lv_font_t font_notosanscjksc_regular_14_b49ae336_fb3f = {
    .get_glyph_dsc = font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_dsc,
    .get_glyph_bitmap = font_notosanscjksc_regular_14_b49ae336_fb3f_get_glyph_bitmap,
    .line_height = 17,
    .base_line = 4,
    .subpx = 0,
    .underline_position = 0,
    .underline_thickness = 0,
    .dsc = NULL,
    .fallback = NULL,
};
