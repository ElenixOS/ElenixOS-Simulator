/**
 * @file eos_dump_port.c
 * @brief Native simulator dump capture
 */

/* Includes ---------------------------------------------------*/
#if !defined(__EMSCRIPTEN__) && (defined(__unix__) || defined(__APPLE__))
#include <execinfo.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include "eos_dump.h"

/* Macros and Definitions -------------------------------------*/

/* Variables --------------------------------------------------*/

/* Function Implementations -----------------------------------*/
#if !defined(__EMSCRIPTEN__) && (defined(__unix__) || defined(__APPLE__))
static uintptr_t _native_stack_pointer(void)
{
#if defined(__x86_64__)
    uintptr_t value;
    __asm__ volatile("mov %%rsp, %0" : "=r"(value));
    return value;
#elif defined(__aarch64__)
    uintptr_t value;
    __asm__ volatile("mov %0, sp" : "=r"(value));
    return value;
#elif defined(__i386__)
    uintptr_t value;
    __asm__ volatile("mov %%esp, %0" : "=r"(value));
    return value;
#else
    return (uintptr_t)__builtin_frame_address(0);
#endif
}

void eos_port_dump_capture(eos_dump_context_t *context)
{
    void *frames[EOS_DUMP_MAX_BACKTRACE + 3U];
    const uintptr_t *stack;
    int frame_count;
    size_t index;
    size_t frame_start = 3U;

    if (!context)
    {
        return;
    }

    context->sp = _native_stack_pointer();
    context->fp = (uintptr_t)__builtin_frame_address(0);
    context->stack_address = context->fp;
    context->lr = (uintptr_t)__builtin_return_address(0);

    frame_count = backtrace(frames, (int)(sizeof(frames) / sizeof(frames[0])));
    if (frame_count <= 0)
    {
        return;
    }

    /* backtrace()[3] is the caller of eos_dump() in a normal dump. */
    if ((size_t)frame_count > frame_start)
    {
        context->pc = (uintptr_t)frames[frame_start];
    }
    else
    {
        context->pc = (uintptr_t)frames[0];
        frame_start = 0U;
    }

    for (index = frame_start; index < (size_t)frame_count && context->backtrace_count < EOS_DUMP_MAX_BACKTRACE; index++)
    {
        context->backtrace[context->backtrace_count++] = (uintptr_t)frames[index];
    }

    stack = (const uintptr_t *)context->stack_address;
    for (index = 0U; index < EOS_DUMP_MAX_STACK_WORDS; index++)
    {
        context->stack_words[context->stack_word_count++] = stack[index];
    }
}
#else
void eos_port_dump_capture(eos_dump_context_t *context)
{
    (void)context;
}
#endif
