#include "lv_port_disp.h"
#include "lcd_driver.h"

#define MY_DISP_HOR_RES   320
#define MY_DISP_VER_RES   240
#define MY_DISP_BUF_LINES 120

static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static lv_color_t s_buf_1[MY_DISP_HOR_RES * MY_DISP_BUF_LINES];
static lv_color_t s_buf_2[MY_DISP_HOR_RES * MY_DISP_BUF_LINES];

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void disp_flush_dma_done_cb(void * user_data);
static void disp_rounder_cb(lv_disp_drv_t * disp_drv, lv_area_t * area);

void lv_port_disp_init(void)
{
    lv_disp_draw_buf_init(&s_draw_buf, s_buf_1, s_buf_2,
                          MY_DISP_HOR_RES * MY_DISP_BUF_LINES);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = MY_DISP_HOR_RES;
    s_disp_drv.ver_res = MY_DISP_VER_RES;
    s_disp_drv.flush_cb = disp_flush;
    s_disp_drv.draw_buf = &s_draw_buf;
    s_disp_drv.rounder_cb = disp_rounder_cb;
    lv_disp_drv_register(&s_disp_drv);
}

static void disp_flush_dma_done_cb(void * user_data)
{
    lv_disp_drv_t * disp_drv = (lv_disp_drv_t *)user_data;
    if(disp_drv != NULL) {
        lv_disp_flush_ready(disp_drv);
    }
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint32_t width = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t height = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t byte_count = width * height * sizeof(lv_color_t);
    uint8_t *pixel_bytes = (uint8_t *)color_p;
    uint32_t byte_index;

    for(byte_index = 0; byte_index < byte_count; byte_index += 2) {
        uint8_t byte = pixel_bytes[byte_index];
        pixel_bytes[byte_index] = pixel_bytes[byte_index + 1];
        pixel_bytes[byte_index + 1] = byte;
    }

    lcd_flush_region((uint16_t)area->x1, (uint16_t)area->y1,
                     (uint16_t)area->x2, (uint16_t)area->y2);

    lcd_write_bytes_dma_async(pixel_bytes, byte_count,
                              disp_flush_dma_done_cb, disp_drv);
}

/*
 * Round invalidated areas' Y to 8-line multiples.
 * Merges scattered dirty areas (e.g. meter needle moves) into bigger blocks,
 * reducing SPI region setup + DMA start/stop overhead for steadier FPS.
 */
static void disp_rounder_cb(lv_disp_drv_t * disp_drv, lv_area_t * area)
{
    (void)disp_drv;
    area->y1 = (area->y1 / 8) * 8;
    area->y2 = ((area->y2 + 7) / 8) * 8 - 1;
    if(area->y2 >= MY_DISP_VER_RES) area->y2 = MY_DISP_VER_RES - 1;
}