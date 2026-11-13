#include "lv_port_disp.h"
#include "lcd_driver.h"

#define MY_DISP_HOR_RES   320
#define MY_DISP_VER_RES   240
#define MY_DISP_BUF_LINES 120

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /* 2 for RGB565 */
#define DISP_BUF_SIZE  (MY_DISP_HOR_RES * MY_DISP_BUF_LINES * BYTE_PER_PIXEL)

/* Two buffers for partial rendering, flushed by SPI DMA in the background.
 * uint32_t arrays keep the buffers aligned to LV_DRAW_BUF_ALIGN (4 bytes). */
LV_ATTRIBUTE_MEM_ALIGN
static uint32_t s_buf_1[DISP_BUF_SIZE / sizeof(uint32_t)];
LV_ATTRIBUTE_MEM_ALIGN
static uint32_t s_buf_2[DISP_BUF_SIZE / sizeof(uint32_t)];

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void disp_flush_dma_done_cb(void * user_data);
static void disp_rounder_event_cb(lv_event_t * e);

void lv_port_disp_init(void)
{
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);

    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_buffers(disp, s_buf_1, s_buf_2, sizeof(s_buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_add_event_cb(disp, disp_rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
}

static void disp_flush_dma_done_cb(void * user_data)
{
    lv_display_t * disp = (lv_display_t *)user_data;
    if(disp != NULL) {
        lv_display_flush_ready(disp);
    }
}

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    uint32_t width = (uint32_t)lv_area_get_width(area);
    uint32_t height = (uint32_t)lv_area_get_height(area);
    uint32_t byte_count = width * height * BYTE_PER_PIXEL;

    /* LVGL renders little endian RGB565, the ILI9341 expects the high byte first on SPI */
    lv_draw_rgb565_swap(px_map, width * height);

    lcd_flush_region((uint16_t)area->x1, (uint16_t)area->y1,
                     (uint16_t)area->x2, (uint16_t)area->y2);

    lcd_write_bytes_dma_async(px_map, byte_count,
                              disp_flush_dma_done_cb, disp);
}

/*
 * Round invalidated areas' Y to 8-line multiples (v9: LV_EVENT_INVALIDATE_AREA replaces rounder_cb).
 * Merges scattered dirty areas (e.g. meter needle moves) into bigger blocks,
 * reducing SPI region setup + DMA start/stop overhead for steadier FPS.
 */
static void disp_rounder_event_cb(lv_event_t * e)
{
    lv_area_t * area = (lv_area_t *)lv_event_get_param(e);

    area->y1 = (area->y1 / 8) * 8;
    area->y2 = (area->y2 / 8) * 8 + 7;
    if(area->y2 >= MY_DISP_VER_RES) area->y2 = MY_DISP_VER_RES - 1;
}