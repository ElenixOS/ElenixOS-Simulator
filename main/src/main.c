/**
 * @file main.c
 */

// Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#ifndef __EMSCRIPTEN__
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <unistd.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#endif
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include "elenix_os.h"
#include "eos_log.h"
#include "eos_app.h"
#include "eos_app_list.h"
#include "eos_activity.h"
#include "eos_service_storage.h"
#include "eos_mem.h"
#include "script_engine_core.h"
#include "eos_port_vibrator.h"
#include "eos_port_display.h"
#include "eos_port_power.h"
#include "eos_port_time.h"
#include "eos_port_battery.h"
#include "eos_port_audio.h"
#include "eos_port_sensor.h"
#include "eos_service_sensor.h"
#include "eos_dev_battery.h"

// Macros and Definitions

#define SIMULATOR_CONTAINER_WIDTH 500
#define SIMULATOR_CONTAINER_HEIGHT 520
#ifdef __EMSCRIPTEN__
#define WINDOW_WIDTH EOS_DISPLAY_WIDTH
#define WINDOW_HEIGHT EOS_DISPLAY_HEIGHT
#else
#define WINDOW_WIDTH SIMULATOR_CONTAINER_WIDTH
#define WINDOW_HEIGHT SIMULATOR_CONTAINER_HEIGHT
#endif
#define LV_USE_MOUSE_CURSOR_IMAGE 0

#define _RIGHT_FRAME_X 470

#define CROWN_SRC "A:" ASSETS_PATH "SimulatorCrown.png"
#define CROWN_POS_X _RIGHT_FRAME_X - 3
#define CROWN_POS_Y 130
#define CROWN_WIDTH 25
#define CROWN_HEIGHT 65

#define SIDE_BUTTON_SRC "A:" ASSETS_PATH "SimulatorSideButton.png"
#define SIDE_BUTTON_POS_X _RIGHT_FRAME_X
#define SIDE_BUTTON_POS_Y 290
#define SIDE_BUTTON_WIDTH 13
#define SIDE_BUTTON_HEIGHT 99

#ifndef ASSETS_PATH
#define ASSETS_PATH "/"
#endif

// Variables
lv_obj_t *brightness_mask = NULL;

#ifndef __EMSCRIPTEN__
#define EOS_LOG_LATEST_FILE "tmp/latest.log"
#define EOS_LOG_ARCHIVE_NAME_SIZE 128

static FILE *g_log_file = NULL;
static eos_log_listener_id_t g_log_file_listener_id = -1;

static const char *_log_level_to_str(eos_log_level_t level)
{
  switch (level) {
    case EOS_LOG_LEVEL_DEBUG:
      return "DEBUG";
    case EOS_LOG_LEVEL_INFO:
      return "INFO";
    case EOS_LOG_LEVEL_WARN:
      return "WARN";
    case EOS_LOG_LEVEL_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

static void _log_file_listener(eos_log_level_t level, const char *buf, size_t len, void *user_data)
{
  (void)len;

  FILE *fp = (FILE *)user_data;
  if (!fp || !buf) {
    return;
  }

  fprintf(fp, "[%s] %s\n", _log_level_to_str(level), buf);
  fflush(fp);
}

static void _copy_file(FILE *src, FILE *dst)
{
  char buffer[4096];
  size_t read_len;

  while ((read_len = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    if (fwrite(buffer, 1, read_len, dst) != read_len) {
      break;
    }
  }

  fflush(dst);
}

static void _archive_latest_log(void)
{
  FILE *src = fopen(EOS_LOG_LATEST_FILE, "rb");
  if (!src) {
    return;
  }

  time_t now = time(NULL);
  struct tm tm_now;
  char archive_path[EOS_LOG_ARCHIVE_NAME_SIZE];

#ifdef _WIN32
  if (localtime_s(&tm_now, &now) != 0) {
    fclose(src);
    return;
  }
#else
  if (!localtime_r(&now, &tm_now)) {
    fclose(src);
    return;
  }
#endif

  if (strftime(archive_path, sizeof(archive_path), "tmp/%Y-%m-%d_%H-%M-%S.log", &tm_now) == 0) {
    fclose(src);
    return;
  }

  FILE *dst = fopen(archive_path, "wb");
  if (!dst) {
    fclose(src);
    return;
  }

  _copy_file(src, dst);

  fclose(dst);
  fclose(src);
}

static void _cleanup_log_file(void)
{
  if (g_log_file_listener_id >= 0) {
    eos_log_unregister_listener(g_log_file_listener_id);
    g_log_file_listener_id = -1;
  }

  if (g_log_file) {
    fflush(g_log_file);
    fclose(g_log_file);
    g_log_file = NULL;
  }
}

static void _init_log_file(void)
{
  eos_service_log_init();

#ifdef _WIN32
  _mkdir("tmp");
#else
  mkdir("tmp", 0755);
#endif
  _archive_latest_log();

  g_log_file = fopen(EOS_LOG_LATEST_FILE, "wb");
  if (!g_log_file) {
    return;
  }

  setvbuf(g_log_file, NULL, _IOLBF, 0);
  g_log_file_listener_id = eos_log_register_listener("file_log",
                                                     _log_file_listener,
                                                     g_log_file,
                                                     0);
  if (g_log_file_listener_id < 0) {
    _cleanup_log_file();
    return;
  }

  atexit(_cleanup_log_file);
}
#endif  /* !defined(__EMSCRIPTEN__) */

// Function Implementations
static lv_display_t *hal_init(int32_t w, int32_t h);

extern void freertos_main(void);

#ifdef __EMSCRIPTEN__
static void lock_canvas_size(void)
{
  const uint16_t canvas_width = WINDOW_WIDTH;
  const uint16_t canvas_height = WINDOW_HEIGHT;
  emscripten_set_canvas_element_size("#canvas", canvas_width, canvas_height);
  emscripten_set_element_css_size("#canvas", (double)canvas_width, (double)canvas_height);
}

static EM_BOOL eos_main_loop_frame(double time, void *user_data)
{
  (void)time;
  (void)user_data;

  eos_main_loop();
  return EM_TRUE;
}
#endif

#ifdef _WIN32
#define main SDL_main
#endif

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  lv_init();
  lv_lodepng_init();

#ifndef __EMSCRIPTEN__
  _init_log_file();
#endif

#ifdef __EMSCRIPTEN__
  lock_canvas_size();
#endif

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  hal_init(WINDOW_WIDTH, WINDOW_HEIGHT);

  eos_port_sensor_init();
  eos_service_sensor_init();
  eos_port_vibrator_init();
  eos_port_display_init();
  eos_port_power_init();
  eos_port_time_init();
  eos_port_battery_init();
  eos_port_audio_init();

  eos_init();
#ifdef __EMSCRIPTEN__
  emscripten_request_animation_frame_loop(eos_main_loop_frame, NULL);
#else
  while (1)
  {
    uint32_t d = eos_main_loop();
    usleep(d * 1000);
  }
#endif
  return 0;
}

#ifndef __EMSCRIPTEN__
static void _crown_clicked_cb(lv_event_t *e)
{
  eos_crown_button_report(EOS_BUTTON_STATE_CLICKED);
}

static void _side_button_clicked_cb(lv_event_t *e)
{
  eos_side_button_report(EOS_BUTTON_STATE_CLICKED);
}
#endif

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void eos_wasm_crown_click(void)
{
  eos_crown_button_report(EOS_BUTTON_STATE_CLICKED);
}

static char *g_wasm_last_read_code = NULL;

EMSCRIPTEN_KEEPALIVE void eos_wasm_side_click(void)
{
  eos_side_button_report(EOS_BUTTON_STATE_CLICKED);
}

EMSCRIPTEN_KEEPALIVE int eos_wasm_launch_app_by_id(const char *app_id)
{
  if (!(app_id && app_id[0]))
  {
    return 0;
  }

  return eos_app_launch_immediately(app_id) == EOS_OK ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int eos_wasm_reload_current_script(void)
{
  return script_engine_reload_current_script() == SE_OK ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int eos_wasm_back_to_watchface(void)
{
  return eos_activity_back_to_watchface() == EOS_OK ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int eos_wasm_write_app_main_js(const char *app_id, const char *code)
{
  if (!(app_id && app_id[0] && code))
  {
    return 0;
  }

  if (!eos_app_list_contains(app_id))
  {
    return 0;
  }

  char script_path[PATH_MAX];
  int path_len = snprintf(script_path,
                          sizeof(script_path),
                          EOS_APP_INSTALLED_DIR "%s/" EOS_APP_SCRIPT_ENTRY_FILE_NAME,
                          app_id);
  if (path_len <= 0 || path_len >= (int)sizeof(script_path))
  {
    return 0;
  }

  eos_file_t fp = eos_fs_open_write(script_path);
  if (fp == EOS_FILE_INVALID)
  {
    return 0;
  }

  size_t code_len = strlen(code);
  if (code_len > 0)
  {
    int written = eos_fs_write(fp, code, code_len);
    eos_fs_close(fp);
    return written == (int)code_len ? 1 : 0;
  }

  eos_fs_close(fp);
  return 1;
}

EMSCRIPTEN_KEEPALIVE const char *eos_wasm_read_app_main_js(const char *app_id)
{
  if (g_wasm_last_read_code)
  {
    eos_free(g_wasm_last_read_code);
    g_wasm_last_read_code = NULL;
  }

  if (!(app_id && app_id[0]))
  {
    return NULL;
  }

  if (!eos_app_list_contains(app_id))
  {
    return NULL;
  }

  char script_path[PATH_MAX];
  int path_len = snprintf(script_path,
                          sizeof(script_path),
                          EOS_APP_INSTALLED_DIR "%s/" EOS_APP_SCRIPT_ENTRY_FILE_NAME,
                          app_id);
  if (path_len <= 0 || path_len >= (int)sizeof(script_path))
  {
    return NULL;
  }

  if (!eos_storage_is_file(script_path))
  {
    return NULL;
  }

  g_wasm_last_read_code = eos_storage_read_file(script_path);
  return g_wasm_last_read_code;
}
#endif

typedef struct
{
  int16_t diff;
  lv_indev_state_t state;
} lv_sdl_mousewheel_t;

static void _mouse_wheel_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
  lv_sdl_mousewheel_t *dsc = lv_indev_get_driver_data(indev);

  eos_crown_encoder_report(dsc->diff);
  if (dsc->state == LV_INDEV_STATE_PRESSED)
    eos_crown_button_report(EOS_BUTTON_STATE_CLICKED);
  dsc->diff = 0;
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
#ifdef __EMSCRIPTEN__
static lv_display_t *hal_init(int32_t w, int32_t h)
{
  lv_group_set_default(lv_group_create());

  lv_display_t *disp = lv_sdl_window_create(w, h);
  lv_sdl_window_set_resizeable(disp, false);
  lv_sdl_window_set_title(disp, "ElenixOS Simulator");

#if LV_USE_SYSMON
#if LV_USE_PERF_MONITOR
  lv_sysmon_hide_performance(disp);
#endif /*LV_USE_PERF_MONITOR*/
#if LV_USE_MEM_MONITOR
  lv_sysmon_hide_memory(disp);
#endif /*LV_USE_MEM_MONITOR*/
#endif /* LV_USE_SYSMON */
  lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, 0);

  lv_indev_t *mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, lv_group_get_default());
  lv_indev_set_display(mouse, disp);
  lv_display_set_default(disp);

#if LV_USE_MOUSE_CURSOR_IMAGE
  LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  lv_obj_t *cursor_obj;
  cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
  lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
  lv_indev_set_cursor(mouse, cursor_obj);           /*Connect the image  object to the driver*/
#endif

  lv_indev_t *mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_read_cb(mousewheel, _mouse_wheel_read_cb);

  lv_indev_t *kb = lv_sdl_keyboard_create();
  lv_indev_set_display(kb, disp);
  lv_indev_set_group(kb, lv_group_get_default());

  /* Create simulator container like native mode */
  lv_obj_t *simulator_container = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(simulator_container);
  lv_obj_set_size(simulator_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  lv_obj_remove_flag(simulator_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(simulator_container);

  /* Create virtual display container like native mode */
  lv_obj_t *vd_container = lv_obj_create(simulator_container);
  lv_obj_remove_style_all(vd_container);
  lv_obj_set_size(vd_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  lv_obj_center(vd_container);
  lv_obj_remove_flag(vd_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(vd_container, EOS_DISPLAY_RADIUS, 0);
  lv_obj_set_style_clip_corner(vd_container, true, 0);

  /* Create virtual display on vd_container */
  disp = eos_virtual_display_create(vd_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  LV_ASSERT(disp != NULL);
  lv_display_set_default(disp);

#if LV_USE_PERF_MONITOR
  lv_sysmon_hide_performance(disp);
  lv_sysmon_hide_memory(disp);
#endif /* LV_USE_PERF_MONITOR */

  /* Create brightness mask after virtual display (on top of vd, similar to native mode) */
  brightness_mask = lv_obj_create(simulator_container);
  lv_obj_set_size(brightness_mask, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  lv_obj_set_style_bg_color(brightness_mask, lv_color_black(), 0);
  lv_obj_set_style_border_width(brightness_mask, 0, 0);
  lv_obj_set_style_opa(brightness_mask, LV_OPA_TRANSP, 0);
  lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(brightness_mask, EOS_DISPLAY_RADIUS, 0);
  lv_obj_center(brightness_mask);

  return disp;
}
#else
static lv_display_t *hal_init(int32_t w, int32_t h)
{
  lv_group_set_default(lv_group_create());

  lv_display_t *disp = lv_sdl_window_create(w, h);
  lv_sdl_window_set_resizeable(disp, false);
  lv_sdl_window_set_title(disp, "ElenixOS Simulator");

  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);

  lv_indev_t *mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, lv_group_get_default());
  lv_indev_set_display(mouse, disp);
  lv_display_set_default(disp);

#if LV_USE_MOUSE_CURSOR_IMAGE
  LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  lv_obj_t *cursor_obj;
  cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
  lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
  lv_indev_set_cursor(mouse, cursor_obj);           /*Connect the image  object to the driver*/
#endif

  lv_indev_t *mousewheel = lv_sdl_mousewheel_create();
  // lv_indev_set_display(mousewheel, disp);
  // lv_indev_set_group(mousewheel, lv_group_get_default());
  lv_indev_set_read_cb(mousewheel, _mouse_wheel_read_cb);

  lv_indev_t *kb = lv_sdl_keyboard_create();
  lv_indev_set_display(kb, disp);
  lv_indev_set_group(kb, lv_group_get_default());

  lv_obj_t *simulator_container = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(simulator_container);
  lv_obj_set_size(simulator_container, SIMULATOR_CONTAINER_WIDTH, SIMULATOR_CONTAINER_HEIGHT);
  lv_obj_remove_flag(simulator_container, LV_OBJ_FLAG_SCROLLABLE);

  const uint16_t frame_width = 20;
  const uint16_t frame_outline_width = 10;
  const uint16_t watch_frame_width = EOS_DISPLAY_WIDTH + (frame_width) * 2;
  const uint16_t watch_frame_height = EOS_DISPLAY_HEIGHT + (frame_width) * 2;

  lv_obj_t *watch_frame = lv_obj_create(simulator_container);
  lv_obj_remove_style_all(watch_frame);
  lv_obj_set_size(watch_frame, watch_frame_width, watch_frame_height);
  lv_obj_center(watch_frame);
  lv_obj_set_style_bg_color(watch_frame, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(watch_frame, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(watch_frame, EOS_DISPLAY_RADIUS + frame_width, 0);
  lv_obj_set_style_outline_color(watch_frame, lv_color_hex(0x1f1f1f), 0);
  lv_obj_set_style_outline_opa(watch_frame, LV_OPA_COVER, 0);
  lv_obj_set_style_outline_width(watch_frame, frame_outline_width, 0);
  lv_obj_set_style_outline_pad(watch_frame, -2, 0);

  lv_obj_t *vd_container = lv_obj_create(simulator_container);
  lv_obj_remove_style_all(vd_container);
  lv_obj_set_size(vd_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  lv_obj_center(vd_container);
  lv_obj_remove_flag(vd_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(vd_container, EOS_DISPLAY_RADIUS, 0);
  lv_obj_set_style_clip_corner(vd_container, true, 0);

  disp = eos_virtual_display_create(vd_container, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  LV_ASSERT(disp != NULL);
  lv_display_set_default(disp);
#if LV_USE_PERF_MONITOR
  lv_sysmon_hide_performance(disp);
  lv_sysmon_hide_memory(disp);
#endif /* LV_USE_PERF_MONITOR */

  /* Create brightness mask after virtual display (on top of vd) */
  brightness_mask = lv_obj_create(simulator_container);
  lv_obj_set_size(brightness_mask, EOS_DISPLAY_WIDTH, EOS_DISPLAY_HEIGHT);
  lv_obj_set_style_bg_color(brightness_mask, lv_color_black(), 0);
  lv_obj_set_style_border_width(brightness_mask, 0, 0);
  lv_obj_set_style_opa(brightness_mask, LV_OPA_TRANSP, 0);
  lv_obj_remove_flag(brightness_mask, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(brightness_mask, EOS_DISPLAY_RADIUS, 0);
  lv_obj_center(brightness_mask);

  lv_obj_t *crown = lv_imagebutton_create(simulator_container);
  lv_obj_set_pos(crown, CROWN_POS_X, CROWN_POS_Y);
  lv_imagebutton_set_src(crown, LV_IMAGEBUTTON_STATE_RELEASED, CROWN_SRC, CROWN_SRC, CROWN_SRC);
  lv_obj_set_size(crown, CROWN_WIDTH, CROWN_HEIGHT);

  static lv_style_t style_pressed;
  lv_style_init(&style_pressed);
  lv_style_set_image_recolor_opa(&style_pressed, LV_OPA_20);
  lv_style_set_image_recolor(&style_pressed, lv_color_white());
  static lv_style_transition_dsc_t tr;
  static lv_style_prop_t props[] = {LV_STYLE_IMAGE_RECOLOR_OPA, 0};
  lv_style_transition_dsc_init(&tr, props, lv_anim_path_linear, 100, 0, NULL);
  lv_style_set_transition(&style_pressed, &tr);
  lv_obj_add_event_cb(crown, _crown_clicked_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_add_style(crown, &style_pressed, LV_STATE_PRESSED);

  lv_obj_t *side_btn = lv_imagebutton_create(simulator_container);
  lv_obj_set_pos(side_btn, SIDE_BUTTON_POS_X, SIDE_BUTTON_POS_Y);
  lv_obj_set_size(side_btn, SIDE_BUTTON_WIDTH, SIDE_BUTTON_HEIGHT);
  lv_imagebutton_set_src(side_btn, LV_IMAGEBUTTON_STATE_RELEASED, SIDE_BUTTON_SRC, SIDE_BUTTON_SRC, SIDE_BUTTON_SRC);
  lv_obj_add_style(side_btn, &style_pressed, LV_STATE_PRESSED);
  lv_obj_add_event_cb(side_btn, _side_button_clicked_cb, LV_EVENT_CLICKED, NULL);

  return disp;
}
#endif
