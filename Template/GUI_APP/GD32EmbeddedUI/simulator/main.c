/* simulator/main.c — PC simulator entry */
#include "aic_ui.h"
#include "../generated/ui_init.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>

#define SIMULATOR_WIDTH 320
#define SIMULATOR_HEIGHT 240
#define SIMULATOR_KEY_QUEUE_SIZE 64
#define SIMULATOR_WINDOW_STYLE (WS_CAPTION | WS_SYSMENU)
#define SIMULATOR_DEFAULT_TICKS 60
#define SIMULATOR_DEFAULT_TICK_MS 33

static HWND g_hwnd;
static uint32_t g_framebuffer[SIMULATOR_WIDTH * SIMULATOR_HEIGHT];
static lv_disp_draw_buf_t g_draw_buf;
static lv_color_t g_draw_buf_1[SIMULATOR_WIDTH * 40];
static lv_point_t g_pointer_point;
static bool g_pointer_pressed;
static lv_group_t *g_input_group;
static lv_obj_t *g_grouped_screen;
static uint32_t g_key_queue[SIMULATOR_KEY_QUEUE_SIZE];
static uint8_t g_key_queue_head;
static uint8_t g_key_queue_tail;
static uint32_t g_last_key;
static bool g_key_release_pending;
static DWORD g_last_tick;
static bool g_headless;

/* Called by LV_ASSERT_HANDLER in lv_conf_custom.h. The default while(1) spin makes the window
 * "not responding"; show a dialog and exit so the failure is visible. */
void gd32_sim_assert_fail(const char *file, int line)
{
  char text[512];
  snprintf(text, sizeof(text),
           "LVGL assertion failed at %s:%d.\n\n"
           "Most likely the LVGL heap (LV_MEM_SIZE) is exhausted or an object pointer is NULL.\n"
           "Raise 'Allocated Memory Size' in System Settings or reduce the number of screens/widgets, then rebuild.",
           file, line);
  fprintf(stderr, "%s\n", text);
  fflush(stderr);
  if (!g_headless) {
    MessageBoxA(NULL, text, "GD32EmbeddedUI Simulator", MB_OK | MB_ICONERROR | MB_TOPMOST);
  }
  /* abort() can hand the process to Windows Error Reporting, which keeps it alive for seconds */
  TerminateProcess(GetCurrentProcess(), 3);
}

static LONG WINAPI simulator_crash_filter(EXCEPTION_POINTERS *info)
{
  DWORD code = info->ExceptionRecord->ExceptionCode;
  fprintf(stderr, "Simulator crashed: exception 0x%08lX at %p\n", (unsigned long)code,
          info->ExceptionRecord->ExceptionAddress);
  fflush(stderr);
  TerminateProcess(GetCurrentProcess(), code);
  return EXCEPTION_EXECUTE_HANDLER;
}

static void simulator_enqueue_key(uint32_t key)
{
  uint8_t next_tail = (uint8_t)((g_key_queue_tail + 1) % SIMULATOR_KEY_QUEUE_SIZE);
  if (next_tail == g_key_queue_head) return;
  g_key_queue[g_key_queue_tail] = key;
  g_key_queue_tail = next_tail;
}

static bool simulator_dequeue_key(uint32_t *key)
{
  if (g_key_queue_head == g_key_queue_tail) return false;
  *key = g_key_queue[g_key_queue_head];
  g_key_queue_head = (uint8_t)((g_key_queue_head + 1) % SIMULATOR_KEY_QUEUE_SIZE);
  return true;
}

static void simulator_group_screen_objects(lv_obj_t *obj)
{
  if (obj == NULL || g_input_group == NULL) return;
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE) || lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE)) {
    lv_group_add_obj(g_input_group, obj);
  }

  uint32_t child_count = lv_obj_get_child_cnt(obj);
  for (uint32_t i = 0; i < child_count; i++) {
    simulator_group_screen_objects(lv_obj_get_child(obj, i));
  }
}

static void simulator_refresh_input_group(void)
{
  if (g_input_group == NULL) return;
  lv_obj_t *active_screen = lv_scr_act();
  if (active_screen == NULL || active_screen == g_grouped_screen) return;
  lv_group_remove_all_objs(g_input_group);
  simulator_group_screen_objects(active_screen);
  g_grouped_screen = active_screen;
}

static void simulator_pointer_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  (void)drv;
  data->point = g_pointer_point;
  data->state = g_pointer_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void simulator_keypad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  (void)drv;
  if (g_key_release_pending) {
    data->key = g_last_key;
    data->state = LV_INDEV_STATE_RELEASED;
    g_key_release_pending = false;
    return;
  }

  if (simulator_dequeue_key(&g_last_key)) {
    data->key = g_last_key;
    data->state = LV_INDEV_STATE_PRESSED;
    g_key_release_pending = true;
    return;
  }

  data->key = g_last_key;
  data->state = LV_INDEV_STATE_RELEASED;
}

static LRESULT CALLBACK simulator_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
  case WM_MOUSEMOVE:
    g_pointer_point.x = (lv_coord_t)GET_X_LPARAM(lparam);
    g_pointer_point.y = (lv_coord_t)GET_Y_LPARAM(lparam);
    return 0;
  case WM_LBUTTONDOWN:
    g_pointer_point.x = (lv_coord_t)GET_X_LPARAM(lparam);
    g_pointer_point.y = (lv_coord_t)GET_Y_LPARAM(lparam);
    g_pointer_pressed = true;
    SetFocus(hwnd);
    SetCapture(hwnd);
    return 0;
  case WM_LBUTTONUP:
    g_pointer_point.x = (lv_coord_t)GET_X_LPARAM(lparam);
    g_pointer_point.y = (lv_coord_t)GET_Y_LPARAM(lparam);
    g_pointer_pressed = false;
    ReleaseCapture();
    return 0;
  case WM_CHAR:
    if (wparam >= 32) simulator_enqueue_key((uint32_t)wparam);
    return 0;
  case WM_KEYDOWN:
    switch (wparam) {
    case VK_BACK:
      simulator_enqueue_key(LV_KEY_BACKSPACE);
      break;
    case VK_RETURN:
      simulator_enqueue_key(LV_KEY_ENTER);
      break;
    case VK_DELETE:
      simulator_enqueue_key(LV_KEY_DEL);
      break;
    case VK_LEFT:
      simulator_enqueue_key(LV_KEY_LEFT);
      break;
    case VK_RIGHT:
      simulator_enqueue_key(LV_KEY_RIGHT);
      break;
    case VK_UP:
      simulator_enqueue_key(LV_KEY_UP);
      break;
    case VK_DOWN:
      simulator_enqueue_key(LV_KEY_DOWN);
      break;
    case VK_HOME:
      simulator_enqueue_key(LV_KEY_HOME);
      break;
    case VK_END:
      simulator_enqueue_key(LV_KEY_END);
      break;
    case VK_TAB:
      simulator_enqueue_key((GetKeyState(VK_SHIFT) & 0x8000) ? LV_KEY_PREV : LV_KEY_NEXT);
      break;
    case VK_ESCAPE:
      simulator_enqueue_key(LV_KEY_ESC);
      break;
    default:
      break;
    }
    return 0;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SIMULATOR_WIDTH;
    bmi.bmiHeader.biHeight = -SIMULATOR_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, 0, 0, SIMULATOR_WIDTH, SIMULATOR_HEIGHT,
            0, 0, SIMULATOR_WIDTH, SIMULATOR_HEIGHT,
            g_framebuffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  default:
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

static void simulator_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
  (void)disp_drv;
  for (lv_coord_t y = area->y1; y <= area->y2; y++) {
    for (lv_coord_t x = area->x1; x <= area->x2; x++) {
      if (x >= 0 && x < SIMULATOR_WIDTH && y >= 0 && y < SIMULATOR_HEIGHT) {
        lv_color32_t color32;
        color32.full = lv_color_to32(*color_p);
        g_framebuffer[y * SIMULATOR_WIDTH + x] =
          ((uint32_t)color32.ch.blue) |
          ((uint32_t)color32.ch.green << 8) |
          ((uint32_t)color32.ch.red << 16);
      }
      color_p++;
    }
  }
  if (!g_headless) InvalidateRect(g_hwnd, NULL, FALSE);
  lv_disp_flush_ready(disp_drv);
}

static int simulator_window_init(void)
{
  HINSTANCE instance = GetModuleHandle(NULL);
  WNDCLASS wc;
  ZeroMemory(&wc, sizeof(wc));
  wc.lpfnWndProc = simulator_wnd_proc;
  wc.hInstance = instance;
  wc.lpszClassName = "GD32EmbeddedUISimulator";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  RegisterClass(&wc);

  RECT rect = { 0, 0, SIMULATOR_WIDTH, SIMULATOR_HEIGHT };
  AdjustWindowRect(&rect, SIMULATOR_WINDOW_STYLE, FALSE);
  g_hwnd = CreateWindowEx(0, wc.lpszClassName, "GD32EmbeddedUI Simulator",
              SIMULATOR_WINDOW_STYLE, CW_USEDEFAULT, CW_USEDEFAULT,
              rect.right - rect.left, rect.bottom - rect.top,
              NULL, NULL, instance, NULL);
  if (!g_hwnd) return 0;
  ShowWindow(g_hwnd, SW_SHOW);
  UpdateWindow(g_hwnd);
  SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
  return 1;
}

static void simulator_display_init(void)
{
  lv_disp_draw_buf_init(&g_draw_buf, g_draw_buf_1, NULL, SIMULATOR_WIDTH * 40);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SIMULATOR_WIDTH;
  disp_drv.ver_res = SIMULATOR_HEIGHT;
  disp_drv.flush_cb = simulator_flush_cb;
  disp_drv.draw_buf = &g_draw_buf;
  lv_disp_drv_register(&disp_drv);
}

static void simulator_input_init(void)
{
  g_input_group = lv_group_create();
  lv_group_set_default(g_input_group);

  static lv_indev_drv_t pointer_drv;
  lv_indev_drv_init(&pointer_drv);
  pointer_drv.type = LV_INDEV_TYPE_POINTER;
  pointer_drv.read_cb = simulator_pointer_read_cb;
  lv_indev_drv_register(&pointer_drv);
  if (pointer_drv.read_timer != NULL) lv_timer_set_period(pointer_drv.read_timer, 5);

  static lv_indev_drv_t keypad_drv;
  lv_indev_drv_init(&keypad_drv);
  keypad_drv.type = LV_INDEV_TYPE_KEYPAD;
  keypad_drv.read_cb = simulator_keypad_read_cb;
  lv_indev_t *keypad = lv_indev_drv_register(&keypad_drv);
  lv_indev_set_group(keypad, g_input_group);
  if (keypad_drv.read_timer != NULL) lv_timer_set_period(keypad_drv.read_timer, 5);
}

static void simulator_tick_inc_elapsed(void)
{
  DWORD now = GetTickCount();
  DWORD elapsed = now - g_last_tick;
  if (elapsed == 0) return;
  g_last_tick = now;
  lv_tick_inc(elapsed);
}

static int gd32_write_ppm(const char *path, const uint32_t *pixels, int width, int height)
{
  FILE *fp = fopen(path, "wb");
  if (fp == NULL) return 0;
  fprintf(fp, "P6\n%d %d\n255\n", width, height);
  for (int i = 0; i < width * height; i++) {
    uint32_t pixel = pixels[i];
    unsigned char rgb[3];
    rgb[0] = (unsigned char)((pixel >> 16) & 0xFF);
    rgb[1] = (unsigned char)((pixel >> 8) & 0xFF);
    rgb[2] = (unsigned char)(pixel & 0xFF);
    if (fwrite(rgb, 1, 3, fp) != 3) {
      fclose(fp);
      return 0;
    }
  }
  fclose(fp);
  return 1;
}

static const char *simulator_opt_value(int argc, char **argv, const char *name)
{
  for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  return NULL;
}

/* Headless screenshot: advance by a fixed tick step instead of wall-clock time so every run of a project renders byte-identical frames */
static int simulator_run_screenshot(const char *out_path, int ticks, int tick_ms,
                    const char *screen_name, const char *lang)
{
  g_headless = true;
  simulator_display_init();
  simulator_input_init();
  ui_init();

  if (lang != NULL) {
    int lang_id = aic_find_language(lang);
    if (lang_id >= 0) ui_set_language((lang_type_t)lang_id);
  }

  if (screen_name != NULL) {
    lv_obj_t *screen = ui_get_screen_by_name(screen_name);
    if (screen == NULL) {
      fprintf(stderr, "screen not found: %s\n", screen_name);
      return 2;
    }
    lv_scr_load(screen);
  }

  simulator_refresh_input_group();
  for (int i = 0; i < ticks; i++) {
    lv_tick_inc((uint32_t)tick_ms);
    lv_timer_handler();
  }

  if (!gd32_write_ppm(out_path, g_framebuffer, SIMULATOR_WIDTH, SIMULATOR_HEIGHT)) {
    fprintf(stderr, "failed to write %s\n", out_path);
    return 3;
  }
  return 0;
}

int main(int argc, char **argv)
{
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
  SetUnhandledExceptionFilter(simulator_crash_filter);
  lv_init();

  const char *screenshot = simulator_opt_value(argc, argv, "--screenshot");
  if (screenshot != NULL) {
    const char *ticks_opt = simulator_opt_value(argc, argv, "--ticks");
    const char *tick_ms_opt = simulator_opt_value(argc, argv, "--tick-ms");
    int ticks = ticks_opt != NULL ? atoi(ticks_opt) : SIMULATOR_DEFAULT_TICKS;
    int tick_ms = tick_ms_opt != NULL ? atoi(tick_ms_opt) : SIMULATOR_DEFAULT_TICK_MS;
    if (ticks <= 0) ticks = SIMULATOR_DEFAULT_TICKS;
    if (tick_ms <= 0) tick_ms = SIMULATOR_DEFAULT_TICK_MS;
    return simulator_run_screenshot(screenshot, ticks, tick_ms,
                    simulator_opt_value(argc, argv, "--screen"),
                    simulator_opt_value(argc, argv, "--lang"));
  }

  if (!simulator_window_init()) return 1;
  simulator_display_init();
  simulator_input_init();
  ui_init();
  simulator_refresh_input_group();
  g_last_tick = GetTickCount();

  MSG msg;
  while (1) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) return 0;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    simulator_refresh_input_group();
    simulator_tick_inc_elapsed();
    lv_timer_handler();
    Sleep(1);
  }
}
