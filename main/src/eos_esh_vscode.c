/**
 * @file eos_esh_vscode.c
 * @brief VSCode integrated terminal frontend for ESH
 */

/* Includes ---------------------------------------------------*/
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "eos_esh_vscode.h"
#include "esh.h"
#include "esh_log_bridge.h"

/* Macros and Definitions -------------------------------------*/
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
#define EOS_ESH_VSCODE_INPUT_BUFFER_SIZE 64U
#endif

/* Variables --------------------------------------------------*/
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
static esh_t s_esh;
static esh_owner_token_t s_owner;
static bool s_initialized;
static bool s_stdin_closed;
static struct termios s_saved_termios;
static bool s_termios_saved;
static uint8_t s_input_buffer[EOS_ESH_VSCODE_INPUT_BUFFER_SIZE];
static const esh_frontend_t s_frontend;
#endif

/* Function Implementations -----------------------------------*/

#if !defined(__EMSCRIPTEN__) && !defined(_WIN32)
static size_t _eos_esh_vscode_write(const uint8_t *data, size_t length, void *user_data)
{
    size_t written = 0;

    (void)user_data;
    while (written < length)
    {
        ssize_t result = write(STDOUT_FILENO, data + written, length - written);
        if (result > 0)
        {
            written += (size_t)result;
        }
        else if (result < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            break;
        }
    }

    return written;
}

static void _eos_esh_vscode_closed(esh_close_reason_t reason, void *user_data)
{
    (void)reason;
    (void)user_data;
    (void)esh_log_bridge_detach();
}

static void _eos_esh_vscode_restore_terminal(void)
{
    if (s_termios_saved)
    {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &s_saved_termios);
        s_termios_saved = false;
    }
}

static void _eos_esh_vscode_configure_terminal(void)
{
    struct termios configured;

    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &s_saved_termios) != 0)
    {
        return;
    }

    configured = s_saved_termios;
    configured.c_lflag &= (tcflag_t) ~(ICANON | ECHO | ECHOE | ECHONL);
    configured.c_cc[VMIN] = 0;
    configured.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &configured) == 0)
    {
        s_termios_saved = true;
        (void)atexit(_eos_esh_vscode_restore_terminal);
    }
}

static int _eos_esh_vscode_exit(esh_cmd_ctx_t *ctx, int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return (int)esh_request_release(ctx);
}

ESH_CMD_EXPORT(exit, _eos_esh_vscode_exit, "release the active ESH frontend");

static const esh_frontend_t s_frontend = {
    .name = "vscode",
    .write = _eos_esh_vscode_write,
    .on_closed = _eos_esh_vscode_closed,
    .user_data = NULL,
};
#endif

eos_result_t eos_esh_vscode_init(void)
{
#if defined(__EMSCRIPTEN__) || defined(_WIN32)
    return EOS_OK;
#else
    eos_result_t result;

    if (s_initialized)
    {
        return EOS_OK;
    }

    result = esh_init(&s_esh);
    if (result != EOS_OK)
    {
        return result;
    }

    _eos_esh_vscode_configure_terminal();
    result = esh_claim(&s_esh, &s_frontend, ESH_CLAIM_TAKEOVER, &s_owner);
    if (result == EOS_OK)
    {
        result = esh_log_bridge_attach(&s_esh);
        if (result == EOS_OK)
        {
            s_initialized = true;
        }
        else
        {
            (void)esh_release(&s_esh, s_owner);
        }
    }

    return result;
#endif
}

void eos_esh_vscode_poll(void)
{
#if defined(__EMSCRIPTEN__) || defined(_WIN32)
    return;
#else
    struct pollfd descriptor = {
        .fd = STDIN_FILENO,
        .events = POLLIN,
        .revents = 0,
    };

    if (!s_initialized || s_stdin_closed)
    {
        return;
    }

    esh_poll(&s_esh);

    while (poll(&descriptor, 1, 0) > 0 && (descriptor.revents & (POLLIN | POLLHUP)) != 0)
    {
        ssize_t length = read(STDIN_FILENO, s_input_buffer, sizeof(s_input_buffer));
        if (length > 0)
        {
            (void)esh_input(&s_esh, s_owner, s_input_buffer, (size_t)length);
            descriptor.revents = 0;
        }
        else if (length == 0)
        {
            s_stdin_closed = true;
        }
        else if (errno != EINTR)
        {
            s_stdin_closed = true;
        }
    }
#endif
}
