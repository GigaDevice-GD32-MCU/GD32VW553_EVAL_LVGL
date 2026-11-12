/* simulator/aic_ui.h — 模拟器平台适配头文件 */

#ifndef AIC_UI_H
#define AIC_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>

#define LV_USE_AIC_SIMULATOR 1

#define CONN(x, y) x#y
#define LVGL_FONT_PATH(y)  CONN(LVGL_STORAGE_PATH"font/",  y)
#define LVGL_IMAGE_PATH(y) CONN(LVGL_STORAGE_PATH"image/", y)
#define LVGL_VIDEO_PATH(y) CONN(LVGL_STORAGE_PATH"video/", y)

#ifdef __cplusplus
}
#endif

#endif /* AIC_UI_H */
