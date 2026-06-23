#include "eos_debugger.h"
#include "eos_throttler.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>

#define DBG_W 480
#define DBG_H 520
#define WATCH_W 500
#define EXPANDED_W (WATCH_W + DBG_W)
#define COLLAPSED_W WATCH_W

#define PAD  10
#define ROWH 22
#define LBLW 90
#define SLDW 220
#define VALX (PAD + LBLW + SLDW + 4)

typedef struct {
    lv_obj_t *label;
    lv_obj_t *slider;
    lv_obj_t *value_label;
    const char *unit;
} sld_t;

typedef struct {
    lv_obj_t *label;
    lv_obj_t *sw;
} sw_t;

static lv_display_t *g_disp = NULL;
static lv_obj_t *g_outer = NULL;
static lv_obj_t *g_watch = NULL;
static lv_obj_t *g_box = NULL;
static bool g_expanded = false;
static bool g_initialized = false;

static sld_t g_sld[10]; static int g_ns = 0;
static sw_t  g_sw[10];  static int g_nw = 0;

static lv_obj_t *g_s_slow   = NULL;
static lv_obj_t *g_s_ticks  = NULL;
static lv_obj_t *g_s_flash  = NULL;
static lv_obj_t *g_s_ram    = NULL;
static lv_obj_t *g_s_rfail  = NULL;

static uint32_t g_upd = 0;
static eos_throttler_config_t g_cfg;
static lv_obj_t *g_master_label = NULL;

/* ── helpers ── */

static void cfg_r(void);
static bool swv(sw_t *s);

static void _apply(void) { cfg_r(); eos_throttler_set_config(&g_cfg); }

static void _update_disable(void) {
    bool m = eos_throttler_get_master_enable();
    bool cpu = m && swv(&g_sw[0]);
    bool fl  = m && swv(&g_sw[4]);
    bool mem = m && swv(&g_sw[5]);
    bool sys = m && swv(&g_sw[6]);

    /* category switches disabled when master off */
    for (int i = 0; i < g_nw; i++) {
        if (m) lv_obj_clear_state(g_sw[i].sw, LV_STATE_DISABLED);
        else   lv_obj_add_state(g_sw[i].sw, LV_STATE_DISABLED);
    }
    /* sub-switches (FPU/ICache/DCache) additionally follow CPU */
    for (int i = 1; i <= 3; i++) {
        if (cpu) lv_obj_clear_state(g_sw[i].sw, LV_STATE_DISABLED);
        else     lv_obj_add_state(g_sw[i].sw, LV_STATE_DISABLED);
    }

    /* sliders follow master + their category */
    if (cpu) lv_obj_clear_state(g_sld[0].slider, LV_STATE_DISABLED);
    else     lv_obj_add_state(g_sld[0].slider, LV_STATE_DISABLED);
    for (int i = 1; i <= 3; i++) {
        if (fl) lv_obj_clear_state(g_sld[i].slider, LV_STATE_DISABLED);
        else    lv_obj_add_state(g_sld[i].slider, LV_STATE_DISABLED);
    }
    for (int i = 4; i <= 5; i++) {
        if (mem) lv_obj_clear_state(g_sld[i].slider, LV_STATE_DISABLED);
        else     lv_obj_add_state(g_sld[i].slider, LV_STATE_DISABLED);
    }
    if (sys) lv_obj_clear_state(g_sld[6].slider, LV_STATE_DISABLED);
    else     lv_obj_add_state(g_sld[6].slider, LV_STATE_DISABLED);
}

static void lbl_set(sld_t *s, int32_t v) {
    char b[64];
    snprintf(b, sizeof(b), "%d %s", (int)v, s->unit ? s->unit : "");
    lv_label_set_text(s->value_label, b);
}
static void sld_cb(lv_event_t *e) {
    sld_t *s = lv_event_get_user_data(e);
    lbl_set(s, lv_slider_get_value(lv_event_get_target_obj(e)));
    _apply();
}
static void sw_cb(lv_event_t *e) { (void)e; _apply(); _update_disable(); }

static sld_t *add_sld(lv_obj_t *p, const char *name, int32_t lo, int32_t hi, int32_t def, const char *unit, int32_t *y) {
    sld_t *s = &g_sld[g_ns++];
    s->unit = unit;

    s->label = lv_label_create(p);
    lv_label_set_text(s->label, name);
    lv_obj_set_pos(s->label, PAD, *y);
    lv_obj_set_style_text_font(s->label, &lv_font_montserrat_14, 0);

    s->slider = lv_slider_create(p);
    lv_obj_set_size(s->slider, SLDW, 10);
    lv_obj_set_pos(s->slider, PAD + LBLW, *y + 4);
    lv_slider_set_range(s->slider, lo, hi);
    lv_slider_set_value(s->slider, def, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(s->slider, LV_OPA_30, LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(s->slider, LV_OPA_30, LV_PART_INDICATOR | LV_STATE_DISABLED);
    lv_obj_set_style_bg_opa(s->slider, LV_OPA_30, LV_PART_KNOB | LV_STATE_DISABLED);
    lv_obj_add_event_cb(s->slider, sld_cb, LV_EVENT_VALUE_CHANGED, s);

    s->value_label = lv_label_create(p);
    lv_obj_set_pos(s->value_label, VALX, *y);
    lv_obj_set_style_text_font(s->value_label, &lv_font_montserrat_14, 0);
    lbl_set(s, def);

    *y += ROWH;
    return s;
}

static void add_sw_row(lv_obj_t *p, const char **names, bool *defs, int n, int32_t x0, int32_t *y) {
    for (int i = 0; i < n; i++) {
        sw_t *s = &g_sw[g_nw++];
        int32_t x = x0 + i * 95;
        s->sw = lv_switch_create(p);
        lv_obj_set_pos(s->sw, x, *y);
        lv_obj_set_size(s->sw, 38, 20);
        if (defs[i]) lv_obj_add_state(s->sw, LV_STATE_CHECKED);
        lv_obj_add_event_cb(s->sw, sw_cb, LV_EVENT_VALUE_CHANGED, s);
        s->label = lv_label_create(p);
        lv_label_set_text(s->label, names[i]);
        lv_obj_set_pos(s->label, x + 44, *y + 1);
        lv_obj_set_style_text_font(s->label, &lv_font_montserrat_14, 0);
    }
    *y += ROWH;
}

static void mk_sec(lv_obj_t *p, const char *title, int32_t *y) {
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, title);
    lv_obj_set_pos(l, PAD, *y);
    lv_obj_set_style_text_color(l, lv_color_hex(0x4fc3f7), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    *y += 22;
}

static lv_obj_t *mk_stat(lv_obj_t *p, const char *init, int32_t *y) {
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, init);
    lv_obj_set_pos(l, PAD, *y);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    *y += 18;
    return l;
}

static int32_t sv(sld_t *s)  { return lv_slider_get_value(s->slider); }
static void ss(sld_t *s, int32_t v) { lv_slider_set_value(s->slider, v, LV_ANIM_OFF); lbl_set(s, v); }
static bool swv(sw_t *s)     { return lv_obj_has_state(s->sw, LV_STATE_CHECKED); }
static void sws(sw_t *s, bool v) { if (v) lv_obj_add_state(s->sw, LV_STATE_CHECKED); else lv_obj_clear_state(s->sw, LV_STATE_CHECKED); }

/* ── config io ── */
/* sw[0]=CPULim  sw[1]=FPU  sw[2]=ICache  sw[3]=DCache
   sw[4]=FlashLim  sw[5]=MemLim  sw[6]=SysLim
   sld[0]=Freq  sld[1]=FRd  sld[2]=FWr  sld[3]=FEr
   sld[4]=RAM  sld[5]=Stack  sld[6]=Tick */

static void cfg_r(void) {
    g_cfg.cpu_freq_mhz          = (uint32_t)sv(&g_sld[0]);
    g_cfg.flash_read_speed_kbps  = (uint32_t)sv(&g_sld[1]);
    g_cfg.flash_write_speed_kbps = (uint32_t)sv(&g_sld[2]);
    g_cfg.flash_erase_sector_ms  = (uint32_t)sv(&g_sld[3]);
    g_cfg.max_ram_kb             = (uint32_t)sv(&g_sld[4]);
    g_cfg.max_stack_kb           = (uint32_t)sv(&g_sld[5]);
    g_cfg.tick_rate_hz           = (uint32_t)sv(&g_sld[6]);

    g_cfg.cpu_limit_enabled    = swv(&g_sw[0]);
    g_cfg.fpu_enabled      = swv(&g_sw[1]);
    g_cfg.icache_enabled   = swv(&g_sw[2]);
    g_cfg.dcache_enabled   = swv(&g_sw[3]);
    g_cfg.flash_limit_enabled  = swv(&g_sw[4]);
    g_cfg.memory_limit_enabled = swv(&g_sw[5]);
    g_cfg.system_limit_enabled = swv(&g_sw[6]);

    g_cfg.cache_line_size = g_cfg.dcache_enabled ? 32 : 0;
}
static void cfg_w(const eos_throttler_config_t *c) {
    ss(&g_sld[0], (int32_t)c->cpu_freq_mhz);
    ss(&g_sld[1], (int32_t)c->flash_read_speed_kbps);
    ss(&g_sld[2], (int32_t)c->flash_write_speed_kbps);
    ss(&g_sld[3], (int32_t)c->flash_erase_sector_ms);
    ss(&g_sld[4], (int32_t)c->max_ram_kb);
    ss(&g_sld[5], (int32_t)c->max_stack_kb);
    ss(&g_sld[6], (int32_t)c->tick_rate_hz);

    sws(&g_sw[0], c->cpu_limit_enabled);
    sws(&g_sw[1], c->fpu_enabled);
    sws(&g_sw[2], c->icache_enabled);
    sws(&g_sw[3], c->dcache_enabled);
    sws(&g_sw[4], c->flash_limit_enabled);
    sws(&g_sw[5], c->memory_limit_enabled);
    sws(&g_sw[6], c->system_limit_enabled);
}

/* ── callbacks ── */

static void preset_cb(lv_event_t *e) {
    uint16_t sel = lv_dropdown_get_selected(lv_event_get_target_obj(e));
    eos_throttler_apply_preset((eos_throttle_preset_t)sel);
    eos_throttler_get_config(&g_cfg);
    cfg_w(&g_cfg);
    _update_disable();
}
static void master_cb(lv_event_t *e) {
    bool on = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
    eos_throttler_set_master_enable(on);
    if (g_master_label)
        lv_label_set_text(g_master_label, on ? "ON" : "OFF");
    _update_disable();
}

/* ── stats ── */

void eos_debugger_update(void) {
    if (!g_initialized || !g_expanded) return;
    if (++g_upd < 30) return;
    g_upd = 0;
    if (!g_s_slow) return;

    eos_throttler_stats_t st; eos_throttler_get_stats(&st);
    char b[128];

    snprintf(b, sizeof(b), "Slowdown: %.1fx  |  Tick: %.1f Hz", st.effective_slowdown, st.actual_tick_rate_hz);
    lv_label_set_text(g_s_slow, b);
    snprintf(b, sizeof(b), "Ticks: %llu  |  CPU delay: %.2f s", (unsigned long long)st.total_ticks, (double)st.cpu_delay_us / 1000000.0);
    lv_label_set_text(g_s_ticks, b);
    snprintf(b, sizeof(b), "Flash R: %llu  |  Flash W: %llu", (unsigned long long)st.flash_read_bytes, (unsigned long long)st.flash_write_bytes);
    lv_label_set_text(g_s_flash, b);
    snprintf(b, sizeof(b), "RAM peak: %u KB  |  Limit: %u KB", st.peak_ram_usage_kb, g_cfg.max_ram_kb);
    lv_label_set_text(g_s_ram, b);
    snprintf(b, sizeof(b), "Alloc overflow: %u  (above limit)", st.ram_alloc_failures);
    lv_label_set_text(g_s_rfail, b);
}

/* ── build ── */

static void build_body(lv_obj_t *body) {
    g_ns = 0; g_nw = 0;
    int32_t y = 0;

    /* CPU: sw[0]=CPULim  sw[1]=FPU  sw[2]=ICache  sw[3]=DCache  sld[0]=Freq */
    mk_sec(body, "CPU", &y);
    {
        const char *ns[] = {"CPU"};
        bool ds[] = {g_cfg.cpu_limit_enabled};
        add_sw_row(body, ns, ds, 1, PAD + LBLW, &y);
    }
    add_sld(body, "Freq (MHz)", 1, 1000, (int32_t)g_cfg.cpu_freq_mhz, "MHz", &y);
    {
        const char *ns[] = {"FPU", "ICache", "DCache"};
        bool ds[] = {g_cfg.fpu_enabled, g_cfg.icache_enabled, g_cfg.dcache_enabled};
        add_sw_row(body, ns, ds, 3, PAD + LBLW, &y);
    }
    y += 3;

    /* Flash: sw[4]=FlashLim  sld[1]=FRd  sld[2]=FWr  sld[3]=FEr */
    mk_sec(body, "Flash", &y);
    {
        const char *ns[] = {"Flash"};
        bool ds[] = {g_cfg.flash_limit_enabled};
        add_sw_row(body, ns, ds, 1, PAD + LBLW, &y);
    }
    add_sld(body, "Read (KB/s)", 10, 50000, (int32_t)g_cfg.flash_read_speed_kbps, "KB/s", &y);
    add_sld(body, "Write (KB/s)", 10, 10000, (int32_t)g_cfg.flash_write_speed_kbps, "KB/s", &y);
    add_sld(body, "Erase (ms)", 1, 500, (int32_t)g_cfg.flash_erase_sector_ms, "ms", &y);
    y += 3;

    /* Memory: sw[5]=MemLim  sld[4]=RAM  sld[5]=Stack */
    mk_sec(body, "Memory", &y);
    {
        const char *ns[] = {"Memory"};
        bool ds[] = {g_cfg.memory_limit_enabled};
        add_sw_row(body, ns, ds, 1, PAD + LBLW, &y);
    }
    add_sld(body, "RAM (KB)", 16, 4096, (int32_t)g_cfg.max_ram_kb, "KB", &y);
    add_sld(body, "Stack (KB)", 1, 256, (int32_t)g_cfg.max_stack_kb, "KB", &y);
    y += 3;

    /* System: sw[6]=SysLim  sld[6]=Tick */
    mk_sec(body, "System", &y);
    {
        const char *ns[] = {"System"};
        bool ds[] = {g_cfg.system_limit_enabled};
        add_sw_row(body, ns, ds, 1, PAD + LBLW, &y);
    }
    add_sld(body, "Tick (Hz)", 1, 10000, (int32_t)g_cfg.tick_rate_hz, "Hz", &y);
    y += 3;

    /* Stats */
    mk_sec(body, "Stats", &y);
    g_s_slow  = mk_stat(body, "Slowdown: --  |  Tick: -- Hz", &y);
    g_s_ticks = mk_stat(body, "Ticks: --  |  CPU delay: --", &y);
    g_s_flash = mk_stat(body, "Flash R: --  |  Flash W: --", &y);
    g_s_ram   = mk_stat(body, "RAM peak: --  |  Limit: -- KB", &y);
    g_s_rfail = mk_stat(body, "Alloc overflow: --", &y);
}

/* ── public ── */

void eos_debugger_init(lv_display_t *disp, lv_obj_t *outer, lv_obj_t *watch_box) {
    if (g_initialized) return;
    g_disp = disp; g_outer = outer; g_watch = watch_box;
    eos_throttler_get_config(&g_cfg);

    lv_display_t *prev = lv_display_get_default();
    lv_display_set_default(g_disp);

    /* right panel - child of outer */
    g_box = lv_obj_create(g_outer);
    lv_obj_set_size(g_box, DBG_W, DBG_H);
    lv_obj_set_pos(g_box, WATCH_W, 0);
    lv_obj_set_style_bg_color(g_box, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_bg_opa(g_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_box, 0, 0);
    lv_obj_remove_flag(g_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g_box, LV_OBJ_FLAG_HIDDEN);

    /* title - directly on g_box */
    lv_obj_t *t = lv_label_create(g_box);
    lv_label_set_text(t, "ElenixOS Throttler");
    lv_obj_set_pos(t, PAD, 4);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0x4fc3f7), 0);

    /* preset dropdown - directly on g_box */
    lv_obj_t *dd = lv_dropdown_create(g_box);
    lv_dropdown_set_options(dd,
        "None (Full Speed)\nCortex-M0 48MHz\nCortex-M3 72MHz\n"
        "Cortex-M4 168MHz\nCortex-M4F 180MHz\nCortex-M7 480MHz\n"
        "Cortex-M33 160MHz\nRISC-V 144MHz\nESP32 240MHz");
    lv_obj_set_pos(dd, PAD, 28);
    lv_obj_set_size(dd, 200, 28);
    lv_obj_add_event_cb(dd, preset_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* master switch - directly on g_box */
    lv_obj_t *ms = lv_switch_create(g_box);
    lv_obj_set_pos(ms, 218, 28);
    lv_obj_set_size(ms, 38, 20);
    lv_obj_add_event_cb(ms, master_cb, LV_EVENT_VALUE_CHANGED, NULL);
    g_master_label = lv_label_create(g_box);
    lv_label_set_text(g_master_label, "OFF");
    lv_obj_set_pos(g_master_label, 262, 30);
    lv_obj_set_style_text_font(g_master_label, &lv_font_montserrat_14, 0);

    /* scrollable body */
    lv_obj_t *body = lv_obj_create(g_box);
    lv_obj_set_size(body, DBG_W - 4, DBG_H - 50);
    lv_obj_set_pos(body, 2, 48);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(body, LV_DIR_VER);
    lv_obj_set_style_pad_all(body, 0, 0);

    build_body(body);

    _update_disable();

    lv_display_set_default(prev);
    g_initialized = true;
}

void eos_debugger_toggle(void) {
    if (!g_initialized) return;
    g_expanded = !g_expanded;

    /* black out the screen to prevent artifacts during resize */
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
    lv_obj_invalidate(lv_screen_active());

    if (g_expanded) {
        lv_obj_set_size(g_outer, EXPANDED_W, DBG_H);
        lv_obj_align(g_watch, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_clear_flag(g_box, LV_OBJ_FLAG_HIDDEN);
        lv_display_set_resolution(g_disp, EXPANDED_W, DBG_H);
        lv_refr_now(g_disp);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
        g_upd = 0;
    } else {
        lv_obj_add_flag(g_box, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(g_watch, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(g_outer, COLLAPSED_W, DBG_H);
        lv_display_set_resolution(g_disp, COLLAPSED_W, DBG_H);
        lv_refr_now(g_disp);
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
    }
    lv_obj_invalidate(lv_screen_active());
}

bool eos_debugger_is_expanded(void) { return g_expanded; }
void eos_debugger_deinit(void) { g_initialized = false; }
