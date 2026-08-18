/**
 * @file eos_port_sensor.c
 * @brief Sensor port implementation for PC Simulator
 *
 * Simulates real sensor hardware behavior:
 *  - Each sensor type has an independently configurable sample rate (ODR),
 *    enforced by per-sensor last-sample-time tracking in the poll timer.
 *  - Data generation uses waveform synthesis (sine + noise) per sensor
 *    category — IMU (acce/gyro/mag), optical (hr/spo2), environmental
 *    (light/temp/baro), event-driven (proximity/step).
 *  - Warm-up delay is simulated on enable (IMU ~50ms, HR ~200ms) —
 *    data is suppressed during warm-up but the device remains in
 *    DEV_STATE_READY (no BUSY transition to avoid breaking synchronous
 *    test expectations).
 *  - All hardware events (init/deinit/enable/disable/rate-change/data-ready)
 *    are logged with timestamps and sensor identifiers.
 *
 * Architecture:
 *  _sensors[]           Per-sensor config and waveform state (10 types)
 *  _type_to_sensor_idx[] O(1) lookup from eos_sensor_type_t -> _sensors[] index
 *  _generic_ops         Single shared ops table for all registered devices
 *  _sensor_poll_cb()    20ms base timer; gates per-sensor notify on
 *                        elapsed ticks >= 1000/sample_rate_hz
 *
 * @note Device layer pushes data to service layer via eos_sensor_notify()
 */

#include "eos_port_sensor.h"
#include "eos_dev_sensor.h"
#include "eos_service_sensor.h"
#include "lvgl.h"
#include "mac_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

/* ========================================================================
 *  Math & Utility Helpers
 * ======================================================================== */

#define RAND_RANGE(min, max) ((min) + rand() % ((max) - (min) + 1))

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/** Wrap a float phase to [0, limit) to prevent gradual precision drift. */
static inline float _wrap_phase(float p, float limit) {
    while (p >= limit) p -= limit;
    while (p < 0.0f)  p += limit;
    return p;
}

/* ========================================================================
 *  Waveform State Types (per sensor category)
 * ======================================================================== */

typedef struct {
    float phase_x, phase_y, phase_z;
    float freq_x, freq_y, freq_z;   /**< Base frequencies in Hz */
} _imu_wave_t;

typedef struct {
    float phase;                    /**< Slow drift phase */
} _hr_wave_t;

typedef struct {
    float phase;                    /**< Diurnal cycle phase */
} _env_wave_t;                      /**< Light, temperature, barometer */

typedef union {
    _imu_wave_t imu;
    _hr_wave_t  hr;
    _env_wave_t env;
    /* step / proximity are stateless (use rand or monotonic counters) */
} _wave_state_t;

/* ========================================================================
 *  Per-Sensor Configuration
 * ======================================================================== */

typedef struct _sensor_config_t {
    eos_sensor_type_t type;         /**< Sensor type enum */
    const char       *type_name;    /**< Human-readable label for logging */
    bool              hw_enabled;   /**< Hardware enabled (powered on) */
    uint32_t          sample_rate_hz; /**< Configured output data rate (Hz) */
    uint32_t          last_poll_tick; /**< lv_tick_get() at last notify */
    uint32_t          warmup_remaining_ms; /**< Warm-up countdown; 0 = ready */
    eos_sensor_data_t (*generate)(_wave_state_t *ws); /**< Data generator */
    _wave_state_t     wave;         /**< Per-category waveform state */
    eos_dev_sensor_t *dev;          /**< Registered device handle (NULL if none) */
} _sensor_config_t;

/* ========================================================================
 *  Data Generators (waveform synthesis + noise)
 * ======================================================================== */

/** @brief Accelerometer — walking motion simulation.
 *  X/Y: ~2 Hz oscillation at ±500 mg with 2 % noise.
 *  Z:   ~1 Hz ripple on 1 g static gravity. */
static eos_sensor_data_t _gen_acce(_wave_state_t *ws) {
    eos_sensor_data_t d = {0};
    _imu_wave_t *w = &ws->imu;
    float dt = 1.0f / 25.0f;  /* nominal, actual rate controlled by poll timer */
    w->phase_x = _wrap_phase(w->phase_x + w->freq_x * 2.0f * M_PI * dt, 2.0f * M_PI);
    w->phase_y = _wrap_phase(w->phase_y + w->freq_y * 2.0f * M_PI * dt, 2.0f * M_PI);
    w->phase_z = _wrap_phase(w->phase_z + w->freq_z * 2.0f * M_PI * dt, 2.0f * M_PI);
    d.acce.x = (int16_t)(500.0f * sinf(w->phase_x) + RAND_RANGE(-10, 10));
    d.acce.y = (int16_t)(500.0f * sinf(w->phase_y) + RAND_RANGE(-10, 10));
    d.acce.z = (int16_t)(1000.0f + 200.0f * sinf(w->phase_z) + RAND_RANGE(-10, 10));
    return d;
}

/** @brief Gyroscope — wrist rotation simulation (±100 dps). */
static eos_sensor_data_t _gen_gyro(_wave_state_t *ws) {
    eos_sensor_data_t d = {0};
    _imu_wave_t *w = &ws->imu;
    float dt = 1.0f / 25.0f;
    w->phase_x = _wrap_phase(w->phase_x + w->freq_x * 2.0f * M_PI * dt, 2.0f * M_PI);
    w->phase_y = _wrap_phase(w->phase_y + w->freq_y * 2.0f * M_PI * dt, 2.0f * M_PI);
    w->phase_z = _wrap_phase(w->phase_z + w->freq_z * 2.0f * M_PI * dt, 2.0f * M_PI);
    d.gyro.x = (int16_t)(100.0f * sinf(w->phase_x) + RAND_RANGE(-5, 5));
    d.gyro.y = (int16_t)(100.0f * sinf(w->phase_y) + RAND_RANGE(-5, 5));
    d.gyro.z = (int16_t)(100.0f * sinf(w->phase_z) + RAND_RANGE(-5, 5));
    return d;
}

/** @brief Magnetometer — Earth field (~300 mG) with slow orientation drift. */
static eos_sensor_data_t _gen_mag(_wave_state_t *ws) {
    eos_sensor_data_t d = {0};
    _imu_wave_t *w = &ws->imu;
    float dt = 1.0f / 25.0f;
    /* Very slow drift — multiply frequency by 0.1 */
    w->phase_x = _wrap_phase(w->phase_x + w->freq_x * 0.1f * 2.0f * M_PI * dt, 2.0f * M_PI);
    w->phase_y = _wrap_phase(w->phase_y + w->freq_y * 0.1f * 2.0f * M_PI * dt, 2.0f * M_PI);
    w->phase_z = _wrap_phase(w->phase_z + w->freq_z * 0.1f * 2.0f * M_PI * dt, 2.0f * M_PI);
    d.mag.x = (int16_t)(300.0f * sinf(w->phase_x) + RAND_RANGE(-10, 10));
    d.mag.y = (int16_t)(300.0f * sinf(w->phase_y) + RAND_RANGE(-10, 10));
    d.mag.z = (int16_t)(300.0f * sinf(w->phase_z) + RAND_RANGE(-10, 10));
    return d;
}

/** @brief Heart Rate — slowly-drifting base (75±15 bpm) + beat-to-beat variation. */
static eos_sensor_data_t _gen_hr(_wave_state_t *ws) {
    eos_sensor_data_t d = {0};
    _hr_wave_t *w = &ws->hr;
    w->phase = _wrap_phase(w->phase + 0.05f, 1000.0f);
    int16_t base = 80 + (int16_t)(15.0f * sinf(w->phase * 0.3f));
    d.hr.heart_rate = (uint16_t)(base + RAND_RANGE(-2, 2));
    return d;
}

/** @brief SpO2 — 95-100 % with slow variation. */
static eos_sensor_data_t _gen_spo2(_wave_state_t *ws) {
    (void)ws;
    eos_sensor_data_t d = {0};
    d.spo2.spo2 = RAND_RANGE(95, 100);
    return d;
}

/** @brief Ambient Light — diurnal cycle 0-10000 lux + noise. */
static eos_sensor_data_t _gen_light(_wave_state_t *ws) {
    eos_sensor_data_t d = {0};
    _env_wave_t *w = &ws->env;
    w->phase = _wrap_phase(w->phase + 0.001f, 1000.0f);
    float cycle = sinf(w->phase * 0.01f);
    d.light.lux = (uint32_t)(5000.0f + 5000.0f * cycle) + RAND_RANGE(0, 500);
    return d;
}

/** @brief Proximity — mostly far, occasionally near (hand/wrist detection). */
static eos_sensor_data_t _gen_proximity(_wave_state_t *ws) {
    (void)ws;
    eos_sensor_data_t d = {0};
    if (rand() % 10 == 0) {
        d.proximity.distance_mm = RAND_RANGE(0, 20);   /* Near */
    } else {
        d.proximity.distance_mm = RAND_RANGE(50, 100);  /* Far */
    }
    return d;
}

/** @brief Skin Temperature — 32-37 °C (3200-3700 hundredths). */
static eos_sensor_data_t _gen_temp(_wave_state_t *ws) {
    (void)ws;
    eos_sensor_data_t d = {0};
    d.temp.temp = RAND_RANGE(3200, 3700);
    return d;
}

/** @brief Barometer — 990-1010 hPa. */
static eos_sensor_data_t _gen_baro(_wave_state_t *ws) {
    (void)ws;
    eos_sensor_data_t d = {0};
    d.baro.pressure = RAND_RANGE(99000, 101000);
    return d;
}

/** @brief Step Counter — monotonic, occasionally increments by 1-3 steps. */
static eos_sensor_data_t _gen_step(_wave_state_t *ws) {
    (void)ws;
    eos_sensor_data_t d = {0};
    static uint32_t step_count = 0;
    if (rand() % 3 == 0) {
        step_count += RAND_RANGE(1, 3);
    }
    d.step.steps = step_count;
    return d;
}

/* ========================================================================
 *  Sensor Configuration Table
 * ========================================================================
 *
 * DEFAULT_RATE: nominal ODR in Hz for each sensor type. Real hardware
 *   would support a discrete set of rates (e.g. 0.78, 1.56, 3.125, 6.25,
 *   12.5, 25, 50, 100 Hz for IMU). The simulator accepts any uint32_t Hz.
 *
 * WARMUP_MS: approximate time for the sensor analog front-end to
 *   stabilize after power-on. Real values: IMU ~30-50ms, PPG (HR/SpO2)
 *   ~100-500ms depending on LED current and sample averaging.
 */

#define DEFAULT_RATE_IMU   25    /* Accelerometer / Gyro / Mag */
#define DEFAULT_RATE_PPG   1     /* HR / SpO2 — PPG sensors run slower */
#define DEFAULT_RATE_ENV   10    /* Light / Proximity */
#define DEFAULT_RATE_SLOW  1     /* Temperature / Baro / Step */

#define WARMUP_IMU_MS      50
#define WARMUP_PPG_MS      200
#define WARMUP_ENV_MS      30
#define WARMUP_INSTANT_MS  0

static _sensor_config_t _sensors[] = {
    /*  type                       name        enabled  rate             last_tick warmup gen           wave state                      dev  */
    { EOS_SENSOR_TYPE_ACCE,       "Accel",    false, DEFAULT_RATE_IMU,  0, 0,     _gen_acce,     {.imu={0,0,0, 1.7f,2.3f,0.5f}}, NULL },
    { EOS_SENSOR_TYPE_GYRO,       "Gyro",     false, DEFAULT_RATE_IMU,  0, 0,     _gen_gyro,     {.imu={0,0,0, 0.8f,1.1f,1.5f}}, NULL },
    { EOS_SENSOR_TYPE_MAG,        "Mag",      false, DEFAULT_RATE_IMU,  0, 0,     _gen_mag,      {.imu={0,0,0, 0.3f,0.5f,0.4f}}, NULL },
    { EOS_SENSOR_TYPE_HR,         "HR",       false, DEFAULT_RATE_PPG,  0, 0,     _gen_hr,       {.hr={0}},                      NULL },
    { EOS_SENSOR_TYPE_SPO2,       "SpO2",     false, DEFAULT_RATE_PPG,  0, 0,     _gen_spo2,     {0},                            NULL },
    { EOS_SENSOR_TYPE_LIGHT,      "Light",    false, DEFAULT_RATE_ENV,  0, 0,     _gen_light,    {.env={0}},                     NULL },
    { EOS_SENSOR_TYPE_PROXIMITY,  "Proximity",false, DEFAULT_RATE_ENV,  0, 0,     _gen_proximity,{0},                            NULL },
    { EOS_SENSOR_TYPE_TEMP,       "Temp",     false, DEFAULT_RATE_SLOW, 0, 0,     _gen_temp,     {0},                            NULL },
    { EOS_SENSOR_TYPE_BARO,       "Baro",     false, DEFAULT_RATE_SLOW, 0, 0,     _gen_baro,     {0},                            NULL },
    { EOS_SENSOR_TYPE_STEP,       "Step",     false, DEFAULT_RATE_SLOW, 0, 0,     _gen_step,     {0},                            NULL },
};

#define SENSOR_COUNT (sizeof(_sensors) / sizeof(_sensors[0]))

/** Fast O(1) lookup: sensor type → index in _sensors[], -1 = not found. */
static int8_t _type_to_sensor_idx[EOS_SENSOR_TYPE_MAX];

static void _build_type_index(void) {
    memset(_type_to_sensor_idx, -1, sizeof(_type_to_sensor_idx));
    for (size_t i = 0; i < SENSOR_COUNT; i++) {
        if (_sensors[i].type < EOS_SENSOR_TYPE_MAX) {
            _type_to_sensor_idx[_sensors[i].type] = (int8_t)i;
        }
    }
}

/** Lookup sensor config by device; returns NULL if not found. */
static _sensor_config_t *_lookup(eos_dev_sensor_t *dev) {
    if (!dev) return NULL;
    if (dev->type <= EOS_SENSOR_TYPE_UNKNOWN || dev->type >= EOS_SENSOR_TYPE_MAX) return NULL;
    int8_t idx = _type_to_sensor_idx[dev->type];
    if (idx < 0) return NULL;
    return &_sensors[idx];
}

/** Return warm-up time for a sensor type (mirrors real hardware). */
static uint32_t _warmup_ms(eos_sensor_type_t type) {
    switch (type) {
    case EOS_SENSOR_TYPE_ACCE: /* fallthrough */
    case EOS_SENSOR_TYPE_GYRO: /* fallthrough */
    case EOS_SENSOR_TYPE_MAG:  return WARMUP_IMU_MS;
    case EOS_SENSOR_TYPE_HR:   /* fallthrough */
    case EOS_SENSOR_TYPE_SPO2: return WARMUP_PPG_MS;
    case EOS_SENSOR_TYPE_LIGHT:/* fallthrough */
    case EOS_SENSOR_TYPE_PROXIMITY: return WARMUP_ENV_MS;
    default: return WARMUP_INSTANT_MS;
    }
}

/* ========================================================================
 *  Generic Device Operations (shared across all sensor types)
 * ======================================================================== */

static void _generic_init(eos_dev_sensor_t *dev) {
    _sensor_config_t *s = _lookup(dev);
    if (!s) return;

    s->dev = dev;
    s->hw_enabled = false;
    s->warmup_remaining_ms = 0;

    eos_dev_sensor_report_state(dev, DEV_STATE_READY);
    printf("[PortSensor:%s] HW_INIT  | device ready, default ODR=%u Hz\n",
           s->type_name, s->sample_rate_hz);
}

static void _generic_deinit(eos_dev_sensor_t *dev) {
    _sensor_config_t *s = _lookup(dev);
    if (!s) return;

    s->hw_enabled = false;
    s->warmup_remaining_ms = 0;
    s->dev = NULL;

    eos_dev_sensor_report_state(dev, DEV_STATE_NONE);
    printf("[PortSensor:%s] HW_DEINIT | device shutdown\n", s->type_name);
}

static void _generic_enable(eos_dev_sensor_t *dev) {
    _sensor_config_t *s = _lookup(dev);
    if (!s) return;

    if (s->hw_enabled) {
        printf("[PortSensor:%s] HW_ENABLE | already enabled, skipping\n", s->type_name);
        return;
    }

    s->hw_enabled = true;
    s->warmup_remaining_ms = _warmup_ms(s->type);
    s->last_poll_tick = 0;  /* reset so first sample fires immediately after warm-up */

    printf("[PortSensor:%s] HW_ENABLE | powering on, warm-up=%ums, ODR=%u Hz (state=READY)\n",
           s->type_name, s->warmup_remaining_ms, s->sample_rate_hz);
}

static void _generic_disable(eos_dev_sensor_t *dev) {
    _sensor_config_t *s = _lookup(dev);
    if (!s) return;

    if (!s->hw_enabled) {
        printf("[PortSensor:%s] HW_DISABLE | already disabled, skipping\n", s->type_name);
        return;
    }

    s->hw_enabled = false;
    s->warmup_remaining_ms = 0;

    eos_dev_sensor_report_state(dev, DEV_STATE_READY);
    printf("[PortSensor:%s] HW_DISABLE | powered off\n", s->type_name);
}

static void _generic_set_sample_rate(eos_dev_sensor_t *dev, uint32_t hz) {
    _sensor_config_t *s = _lookup(dev);
    if (!s) return;

    uint32_t old_hz = s->sample_rate_hz;
    s->sample_rate_hz = (hz > 0) ? hz : 1;
    s->last_poll_tick = 0;  /* reset to apply new rate immediately */

    printf("[PortSensor:%s] HW_CFG   | ODR changed: %u Hz → %u Hz\n",
           s->type_name, old_hz, s->sample_rate_hz);
}

static void _generic_get_sample_rate(eos_dev_sensor_t *dev, uint32_t *hz) {
    _sensor_config_t *s = _lookup(dev);
    if (s && hz) {
        *hz = s->sample_rate_hz;
    } else if (hz) {
        *hz = 0;
    }
}

static const eos_dev_sensor_ops_t _generic_ops = {
    .init            = _generic_init,
    .deinit          = _generic_deinit,
    .enable          = _generic_enable,
    .disable         = _generic_disable,
    .set_sample_rate = _generic_set_sample_rate,
    .get_sample_rate = _generic_get_sample_rate,
};

/* ========================================================================
 *  Poll Timer
 * ======================================================================== */

/**
 * @brief Base poll period in ms.
 *
 * Must be <= the period of the fastest sensor (currently 25 Hz → 40 ms).
 * Using 20 ms (50 Hz base) allows correct servicing of 25 Hz (2 ticks),
 * 10 Hz (5 ticks), 1 Hz (50 ticks), and future rates up to 50 Hz.
 *
 * On real hardware this would be replaced by per-sensor data-ready
 * interrupts or a sensor-hub FIFO-watermark interrupt.
 */
#define POLL_PERIOD_MS 20

static void _sensor_poll_cb(lv_timer_t *t) {
    (void)t;
    uint32_t now = lv_tick_get();

    for (size_t i = 0; i < SENSOR_COUNT; i++) {
        _sensor_config_t *s = &_sensors[i];

        /* Skip sensors without a registered device */
        if (!s->dev) continue;

        /* Skip if hardware is not enabled */
        if (!s->hw_enabled) continue;

        /* Handle warm-up countdown */
        if (s->warmup_remaining_ms > 0) {
            if (s->warmup_remaining_ms <= POLL_PERIOD_MS) {
                s->warmup_remaining_ms = 0;
                printf("[PortSensor:%s] HW_READY | warm-up complete, starting data at %u Hz\n",
                       s->type_name, s->sample_rate_hz);
            } else {
                s->warmup_remaining_ms -= POLL_PERIOD_MS;
                continue;  /* Still warming up, no data yet */
            }
        }

        /* Rate-limit: check if enough time has elapsed for this sensor's ODR */
        if (s->sample_rate_hz == 0) continue;

        uint32_t interval_ms = 1000 / s->sample_rate_hz;
        if (interval_ms == 0) interval_ms = 1;

        if (s->last_poll_tick != 0) {
            uint32_t elapsed = now - s->last_poll_tick;
            if (elapsed < interval_ms) continue;  /* Not yet time for next sample */
        }

        s->last_poll_tick = now;

        /* Generate and push data */
        if (s->generate) {
            eos_sensor_data_t data = s->generate(&s->wave);
            eos_sensor_notify(s->type, &data, now);
        }
    }
}

/* ========================================================================
 *  Public Interface
 * ======================================================================== */

static lv_timer_t *_poll_timer = NULL;

void eos_port_sensor_init(void) {
    srand((unsigned int)time(NULL));

    /* Build type → index lookup */
    _build_type_index();

    /* Register devices that have simulated hardware in this port */
    eos_dev_sensor_register("sim_acce",  EOS_SENSOR_TYPE_ACCE,  &_generic_ops);
    eos_dev_sensor_register("sim_gyro",  EOS_SENSOR_TYPE_GYRO,  &_generic_ops);
    eos_dev_sensor_register("sim_mag",   EOS_SENSOR_TYPE_MAG,   &_generic_ops);
    eos_dev_sensor_register("sim_hr",    EOS_SENSOR_TYPE_HR,    &_generic_ops);
    eos_dev_sensor_register("sim_spo2",  EOS_SENSOR_TYPE_SPO2,  &_generic_ops);
    eos_dev_sensor_register("sim_light", EOS_SENSOR_TYPE_LIGHT, &_generic_ops);
    eos_dev_sensor_register("sim_temp",  EOS_SENSOR_TYPE_TEMP,  &_generic_ops);
    eos_dev_sensor_register("sim_baro",  EOS_SENSOR_TYPE_BARO,  &_generic_ops);
    eos_dev_sensor_register("sim_step",  EOS_SENSOR_TYPE_STEP,  &_generic_ops);

    /*
     * NOTE: Sensors are NOT enabled by default here. The service layer
     * calls enable() when the first subscriber appears and disable()
     * when the last subscriber disappears. This mirrors real hardware
     * power management where sensors are kept in standby until needed.
     */

    /* Start the poll timer — this represents the "sensor hub" polling loop.
     * On real hardware this would be a combination of GPIO interrupts and
     * a low-power timer for periodic FIFO draining. */
    _poll_timer = lv_timer_create(_sensor_poll_cb, POLL_PERIOD_MS, NULL);
    if (_poll_timer) {
        lv_timer_set_repeat_count(_poll_timer, -1);  /* Run indefinitely */
    }

    printf("[PortSensor] INIT    | %zu sensor types configured, 9 devices registered, poll period=%ums\n",
           SENSOR_COUNT, POLL_PERIOD_MS);
}
