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
#include "mac_api.h"

// Macros and Definitions

// Variables
extern lv_obj_t *brightness_mask;
typedef struct eos_sem_t eos_sem_t;
struct eos_sem_t
{
    pthread_mutex_t mutex;
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
    return malloc(size);
}

void *eos_malloc_zeroed_core(size_t size)
{
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

void eos_locate_phone(void)
{
    printf("Locate phone\n");
}
