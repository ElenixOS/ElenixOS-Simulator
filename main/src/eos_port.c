/**
 * @file eos_port.c
 * @brief ElenixOS Port for PC Simulator
 */

#include "eos_port.h"

// Includes
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include "time.h"
#include <sys/time.h>
#if LV_USE_OS == LV_OS_ELENAOS
#include "eos_core.h"
#include "eos_service_time.h"
#endif
#include "eos_config.h"
#include "eos_throttler.h"
#include "mac_api.h"
#include "eos_service_sensor.h"

// Macros and Definitions

// Variables
extern lv_obj_t *brightness_mask;
typedef struct eos_sem_t eos_sem_t;
struct eos_sem_t
{
    pthread_mutex_t mutex;
};

static eos_dev_sensor_ops_t _sensor_ops = {
    .init = NULL,
    .deinit = NULL,
    .enable = NULL,
    .disable = NULL,
    .set_sample_rate = NULL,
    .get_sample_rate = NULL,
};

// Function Implementations

eos_sem_t *eos_sem_create(uint32_t initial_count, uint32_t max_count)
{
    (void)initial_count;
    (void)max_count;

    eos_sem_t *sem = malloc(sizeof(eos_sem_t));
    if (!sem)
        return NULL;

    if (pthread_mutex_init(&sem->mutex, NULL) != 0)
    {
        free(sem);
        return NULL;
    }

    return sem;
}

void eos_sem_destroy(eos_sem_t *sem)
{
    if (!sem)
        return;

    pthread_mutex_destroy(&sem->mutex);
    free(sem);
}

bool eos_sem_take(eos_sem_t *sem, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!sem)
        return false;

    return pthread_mutex_lock(&sem->mutex) == 0;
}

void eos_sem_give(eos_sem_t *sem)
{
    if (!sem)
        return;

    pthread_mutex_unlock(&sem->mutex);
}

void *eos_malloc_core(size_t size)
{
    if (!eos_throttler_mem_try_alloc(size)) return NULL;
    return malloc(size);
}

void *eos_malloc_zeroed_core(size_t size)
{
    if (!eos_throttler_mem_try_alloc(size)) return NULL;
    return calloc(1, size);
}

void eos_free_core(void *ptr)
{
    free(ptr);
}

void *eos_realloc_core(void *ptr, size_t new_size)
{
    return realloc(ptr, new_size);
}

void *eos_malloc_large(size_t size)
{
    return malloc(size);
}

void eos_free_large(void *ptr)
{
    free(ptr);
}

void eos_delay(uint32_t ms)
{
    ms = eos_throttler_adjust_delay(ms);
    usleep(ms * 1000);
}

void eos_cpu_reset(void)
{
    return;
}

void eos_bluetooth_enable(void)
{
    return;
}

void eos_bluetooth_disable(void)
{
    return;
}

void *_thread_sensor(void *arg)
{
    eos_sensor_type_t type = *(eos_sensor_type_t *)arg;
    eos_sensor_raw_data_t s = {.type = type, .timestamp = eos_tick_get()};

    switch (type)
    {
    case EOS_SENSOR_TYPE_ACCE:
        s.data.acce.x = (rand() % 4000) - 2000;
        s.data.acce.y = (rand() % 4000) - 2000;
        s.data.acce.z = (rand() % 4000) - 2000;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_GYRO:
        s.data.gyro.x = (rand() % 500) - 250;
        s.data.gyro.y = (rand() % 500) - 250;
        s.data.gyro.z = (rand() % 500) - 250;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_MAG:
        s.data.mag.x = (rand() % 2000) - 1000;
        s.data.mag.y = (rand() % 2000) - 1000;
        s.data.mag.z = (rand() % 2000) - 1000;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_TEMP:
        s.data.temp.temp = (rand() % 3500) + 1500;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_BARO:
        s.data.baro.pressure = 101325 + (rand() % 2000 - 1000);
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_LIGHT:
        s.data.light.lux = rand() % 10000;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_PROXIMITY:
        s.data.proximity.distance_mm = rand() % 500;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_HR:
        s.data.hr.heart_rate = rand() % 100;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_SPO2:
        s.data.spo2.spo2 = rand() % 100;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_ECG:
        s.data.ecg.ecg = rand() % 1000;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_CAP:
        s.data.cap.cap = rand() % 100;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_STEP:
        s.data.step.steps += rand() % 5;
        eos_sensor_notify(type, &s.data, s.timestamp);
        break;

    case EOS_SENSOR_TYPE_UNKNOWN:
    default:
        printf("Unknown sensor type: %d\n", type);
        break;
    }

    return NULL;
}

void eos_sensor_read_async(eos_sensor_type_t type)
{
#ifdef __EMSCRIPTEN__
    eos_sensor_type_t local_type = type;
    _thread_sensor(&local_type);
    return;
#else
    pthread_t thread;

    eos_sensor_type_t *arg = malloc(sizeof(eos_sensor_type_t));
    *arg = type;

    int ret = pthread_create(&thread, NULL, _thread_sensor, arg);
    if (ret != 0)
    {
        perror("pthread_create failed");
        free(arg);
    }
#endif
}

void eos_sensor_read_sync(eos_sensor_type_t type, eos_sensor_raw_data_t *out)
{
    if (!out)
        return;

    eos_sensor_raw_data_t sensor = {.type = type, .timestamp = eos_tick_get()};

    switch (type)
    {
    case EOS_SENSOR_TYPE_HR:
        sensor.data.hr.heart_rate = rand() % 100;
        break;
    case EOS_SENSOR_TYPE_SPO2:
        sensor.data.spo2.spo2 = rand() % 100;
        break;
    default:
        break;
    }

    memcpy(out, &sensor, sizeof(eos_sensor_raw_data_t));
}

void eos_locate_phone(void)
{
    printf("Locate phone\n");
}


