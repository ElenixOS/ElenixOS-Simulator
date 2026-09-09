/**
 * @file eos_headless_display.c
 * @brief LVGL display backend without a native window.
 */

#include "eos_headless_display.h"

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include "eos_config.h"
#include "eos_headless_ipc.h"
#include "eos_headless_websocket.h"
#include "eos_crown.h"
#include "eos_side_button.h"
#include "eos_touch.h"

#define EOS_HEADLESS_INPUT_QUEUE_SIZE 64U

typedef struct
{
    eos_headless_websocket_input_action_t action;
    int32_t x;
    int32_t y;
} eos_headless_input_event_t;

typedef struct
{
    /* LVGL owns this buffer while rendering.  Keep it separate from the
     * framebuffer that is published over IPC so a flush cannot overwrite the
     * active draw buffer while LVGL is still using it. */
    uint8_t *lvgl_frame;
    uint8_t *render_frame;
    uint8_t *output_frame;
    size_t frame_size;
    uint32_t stride;
    uint8_t brightness;
    bool power_on;
} eos_headless_display_data_t;

static eos_headless_display_data_t *s_display_data;
static eos_headless_input_event_t s_input_queue[EOS_HEADLESS_INPUT_QUEUE_SIZE];
static uint8_t s_input_queue_head;
static uint8_t s_input_queue_tail;
static uint8_t s_input_queue_count;
static uint32_t s_input_width;
static uint32_t s_input_height;
static int32_t s_input_x;
static int32_t s_input_y;
static lv_indev_state_t s_input_state = LV_INDEV_STATE_RELEASED;

static void _headless_input_reset(uint32_t width, uint32_t height)
{
    s_input_queue_head = 0U;
    s_input_queue_tail = 0U;
    s_input_queue_count = 0U;
    s_input_width = width;
    s_input_height = height;
    s_input_x = 0;
    s_input_y = 0;
    s_input_state = LV_INDEV_STATE_RELEASED;
}

static void _headless_input_submit(eos_headless_websocket_input_action_t action, int32_t x, int32_t y)
{
    eos_headless_input_event_t *last;

    if (!s_display_data || x < 0 || y < 0 || (uint32_t)x >= s_input_width || (uint32_t)y >= s_input_height)
    {
        return;
    }

    /* Pointer moves are high-rate and disposable.  Coalesce only adjacent
     * moves, preserving the down/up edges required by LVGL. */
    if (action == EOS_HEADLESS_WEBSOCKET_INPUT_MOVE && s_input_queue_count > 0U)
    {
        uint8_t last_index =
            (uint8_t)((s_input_queue_tail + EOS_HEADLESS_INPUT_QUEUE_SIZE - 1U) % EOS_HEADLESS_INPUT_QUEUE_SIZE);
        last = &s_input_queue[last_index];
        if (last->action == EOS_HEADLESS_WEBSOCKET_INPUT_MOVE)
        {
            last->x = x;
            last->y = y;
            return;
        }
    }

    if (s_input_queue_count >= EOS_HEADLESS_INPUT_QUEUE_SIZE)
    {
        /* Never discard a release edge.  A full queue can only contain
         * stale samples here, so make room by dropping the oldest one. */
        if (action == EOS_HEADLESS_WEBSOCKET_INPUT_MOVE)
        {
            return;
        }
        s_input_queue_head = (uint8_t)((s_input_queue_head + 1U) % EOS_HEADLESS_INPUT_QUEUE_SIZE);
        s_input_queue_count--;
    }

    s_input_queue[s_input_queue_tail].action = action;
    s_input_queue[s_input_queue_tail].x = x;
    s_input_queue[s_input_queue_tail].y = y;
    s_input_queue_tail = (uint8_t)((s_input_queue_tail + 1U) % EOS_HEADLESS_INPUT_QUEUE_SIZE);
    s_input_queue_count++;
}

static void _headless_button_submit(eos_headless_websocket_input_action_t action)
{
    switch (action)
    {
        case EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_CLICK:
            eos_crown_button_report(EOS_BUTTON_STATE_CLICKED);
            break;
        case EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_LONG_PRESS:
            eos_crown_button_report(EOS_BUTTON_STATE_LONG_PRESSED);
            break;
        case EOS_HEADLESS_WEBSOCKET_BUTTON_SIDE_CLICK:
            eos_side_button_report(EOS_BUTTON_STATE_CLICKED);
            break;
        default:
            break;
    }
}

static void _headless_websocket_input_callback(eos_headless_websocket_input_action_t action,
                                               int32_t x,
                                               int32_t y,
                                               void *user_data)
{
    (void)user_data;
    if (action == EOS_HEADLESS_WEBSOCKET_INPUT_WHEEL)
    {
        eos_crown_encoder_scroll_report(x);
        return;
    }
    if (action >= EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_CLICK)
    {
        _headless_button_submit(action);
    }
    else
    {
        _headless_input_submit(action, x, y);
    }
}

static uint32_t _headless_tick_get(void)
{
#ifdef _WIN32
    static ULONGLONG start;
    if (start == 0)
        start = GetTickCount64();
    return (uint32_t)(GetTickCount64() - start);
#else
    struct timespec now;
    static struct timespec start;

    if (start.tv_sec == 0 && start.tv_nsec == 0)
    {
        (void)clock_gettime(CLOCK_MONOTONIC, &start);
    }
    (void)clock_gettime(CLOCK_MONOTONIC, &now);

    return (uint32_t)((now.tv_sec - start.tv_sec) * 1000LL + (now.tv_nsec - start.tv_nsec) / 1000000LL);
#endif
}

static void _headless_delay(uint32_t milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec duration = {
        .tv_sec = (time_t)(milliseconds / 1000U),
        .tv_nsec = (long)((milliseconds % 1000U) * 1000000U),
    };

    while (nanosleep(&duration, &duration) != 0)
    {
        /* Retry if interrupted by a debugger or signal. */
    }
#endif
}

static uint16_t _scale_rgb565(uint16_t color, uint8_t brightness)
{
    uint32_t red = (color >> 11U) & 0x1FU;
    uint32_t green = (color >> 5U) & 0x3FU;
    uint32_t blue = color & 0x1FU;

    red = (red * brightness + 50U) / 100U;
    green = (green * brightness + 50U) / 100U;
    blue = (blue * brightness + 50U) / 100U;
    return (uint16_t)((red << 11U) | (green << 5U) | blue);
}

static void _headless_prepare_output(eos_headless_display_data_t *data)
{
    size_t pixel_count = data->frame_size / 2U;
    const uint16_t *source = (const uint16_t *)data->render_frame;
    uint16_t *destination = (uint16_t *)data->output_frame;

    if (!data->power_on)
    {
        memset(data->output_frame, 0, data->frame_size);
        return;
    }

    if (data->brightness >= 100U)
    {
        memcpy(data->output_frame, data->render_frame, data->frame_size);
        return;
    }

    for (size_t index = 0; index < pixel_count; index++)
    {
        destination[index] = _scale_rgb565(source[index], data->brightness);
    }
}

static void _headless_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *pixel_map)
{
    eos_headless_display_data_t *data = (eos_headless_display_data_t *)lv_display_get_driver_data(display);
    int32_t width;
    int32_t height;
    uint32_t source_stride;
    uint32_t destination_stride;
    uint8_t *destination;

    if (!data || !area || !pixel_map)
    {
        lv_display_flush_ready(display);
        return;
    }

    width = lv_area_get_width(area);
    height = lv_area_get_height(area);
    source_stride = lv_draw_buf_width_to_stride((uint32_t)width, lv_display_get_color_format(display));
    destination_stride = data->stride;
    destination = data->render_frame + (size_t)area->y1 * destination_stride + (size_t)area->x1 * 2U;
    for (int32_t row = 0; row < height; row++)
    {
        memcpy(destination, pixel_map, (size_t)width * 2U);
        destination += destination_stride;
        pixel_map += source_stride;
    }

    if (lv_display_flush_is_last(display))
    {
        _headless_prepare_output(data);
        (void)eos_headless_ipc_submit_frame(data->output_frame, data->frame_size);
        (void)eos_headless_websocket_submit_frame(data->output_frame, data->frame_size);
    }

    lv_display_flush_ready(display);
}

static void _headless_input_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    if (s_input_queue_count > 0U)
    {
        eos_headless_input_event_t *event = &s_input_queue[s_input_queue_head];
        s_input_x = event->x;
        s_input_y = event->y;
        s_input_state =
            event->action == EOS_HEADLESS_WEBSOCKET_INPUT_UP ? LV_INDEV_STATE_RELEASED : LV_INDEV_STATE_PRESSED;
        s_input_queue_head = (uint8_t)((s_input_queue_head + 1U) % EOS_HEADLESS_INPUT_QUEUE_SIZE);
        s_input_queue_count--;
    }
    data->point.x = (lv_coord_t)s_input_x;
    data->point.y = (lv_coord_t)s_input_y;
    data->state = s_input_state;
}

lv_display_t *eos_headless_display_create(int32_t width,
                                          int32_t height,
                                          const char *socket_path,
                                          uint16_t websocket_port)
{
    lv_display_t *display;
    lv_indev_t *indev;
    eos_headless_display_data_t *data;

    lv_tick_set_cb(_headless_tick_get);
    lv_delay_set_cb(_headless_delay);
    _headless_input_reset((uint32_t)width, (uint32_t)height);
    if (!eos_headless_ipc_init(socket_path, (uint32_t)width, (uint32_t)height))
    {
        return NULL;
    }
    if (!eos_headless_websocket_init(websocket_port,
                                     (uint32_t)width,
                                     (uint32_t)height,
                                     _headless_websocket_input_callback,
                                     NULL)
        || !eos_headless_ipc_set_websocket_endpoint(eos_headless_websocket_endpoint()))
    {
        eos_headless_websocket_shutdown();
        eos_headless_ipc_shutdown();
        return NULL;
    }

    data = (eos_headless_display_data_t *)calloc(1U, sizeof(*data));
    if (!data)
    {
        eos_headless_ipc_shutdown();
        return NULL;
    }

    data->frame_size = (size_t)width * (size_t)height * 2U;
    data->stride = (uint32_t)width * 2U;
    data->brightness = 100U;
    data->power_on = true;
    data->lvgl_frame = (uint8_t *)calloc(1U, data->frame_size);
    data->render_frame = (uint8_t *)calloc(1U, data->frame_size);
    data->output_frame = (uint8_t *)calloc(1U, data->frame_size);
    if (!data->lvgl_frame || !data->render_frame || !data->output_frame)
    {
        free(data->lvgl_frame);
        free(data->render_frame);
        free(data->output_frame);
        free(data);
        eos_headless_ipc_shutdown();
        return NULL;
    }

    display = lv_display_create(width, height);
    if (!display)
    {
        free(data->lvgl_frame);
        free(data->render_frame);
        free(data->output_frame);
        free(data);
        eos_headless_ipc_shutdown();
        return NULL;
    }

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, data->lvgl_frame, NULL, data->frame_size, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_driver_data(display, data);
    lv_display_set_flush_cb(display, _headless_flush_cb);
    lv_display_set_default(display);

    indev = lv_indev_create();
    if (!indev)
    {
        lv_display_delete(display);
        free(data->lvgl_frame);
        free(data->render_frame);
        free(data->output_frame);
        free(data);
        eos_headless_ipc_shutdown();
        return NULL;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _headless_input_read);
    lv_indev_set_display(indev, display);
    if (!eos_touch_bind_indev(indev))
    {
        lv_indev_delete(indev);
        lv_display_delete(display);
        free(data->lvgl_frame);
        free(data->render_frame);
        free(data->output_frame);
        free(data);
        eos_headless_ipc_shutdown();
        eos_headless_websocket_shutdown();
        return NULL;
    }

    s_display_data = data;
    return display;
}

void eos_headless_display_set_brightness(uint8_t brightness)
{
    if (s_display_data)
    {
        s_display_data->brightness = brightness > 100U ? 100U : brightness;
    }
}

void eos_headless_display_set_power(bool on)
{
    if (s_display_data)
    {
        s_display_data->power_on = on;
    }
}
