/**
 * @file eos_esh_vscode.c
 * @brief VSCode integrated terminal frontend for ESH
 */

/* Includes ---------------------------------------------------*/
#if !defined(__EMSCRIPTEN__)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <conio.h>
#include <windows.h>
#else
#include <errno.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif
#endif

#include "eos_esh_vscode.h"
#include "esh.h"
#include "esh_log_bridge.h"

/* Macros and Definitions -------------------------------------*/
#if !defined(__EMSCRIPTEN__)
#define EOS_ESH_VSCODE_INPUT_BUFFER_SIZE 64U
#endif

/* Variables --------------------------------------------------*/
#if !defined(__EMSCRIPTEN__)
static esh_t s_esh;
static esh_owner_token_t s_owner;
static bool s_initialized;
static bool s_stdin_closed;
static uint8_t s_input_buffer[EOS_ESH_VSCODE_INPUT_BUFFER_SIZE];
static const esh_frontend_t s_frontend;

#ifdef _WIN32
static HANDLE s_stdin_handle = INVALID_HANDLE_VALUE;
static DWORD s_saved_console_mode;
static bool s_console_mode_saved;
#else
static struct termios s_saved_termios;
static bool s_termios_saved;
#endif
#endif

/* Function Implementations -----------------------------------*/

#if !defined(__EMSCRIPTEN__)
static size_t _eos_esh_vscode_write(const uint8_t *data, size_t length, void *user_data)
{
    size_t written = 0U;

    (void)user_data;
#ifdef _WIN32
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output_handle == NULL || output_handle == INVALID_HANDLE_VALUE)
    {
        return 0U;
    }

    while (written < length)
    {
        DWORD chunk = 0U;
        DWORD requested = (DWORD)((length - written) > UINT32_MAX ? UINT32_MAX : (length - written));

        if (!WriteFile(output_handle, data + written, requested, &chunk, NULL) || chunk == 0U)
        {
            break;
        }
        written += (size_t)chunk;
    }
#else
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
#endif

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
#ifdef _WIN32
    if (s_console_mode_saved && s_stdin_handle != INVALID_HANDLE_VALUE)
    {
        (void)SetConsoleMode(s_stdin_handle, s_saved_console_mode);
        s_console_mode_saved = false;
    }
#else
    if (s_termios_saved)
    {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &s_saved_termios);
        s_termios_saved = false;
    }
#endif
}

static void _eos_esh_vscode_configure_terminal(void)
{
#ifdef _WIN32
    DWORD mode;

    s_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (s_stdin_handle == NULL || s_stdin_handle == INVALID_HANDLE_VALUE || GetConsoleMode(s_stdin_handle, &mode) == 0)
    {
        return;
    }

    s_saved_console_mode = mode;
    mode &= (DWORD) ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (SetConsoleMode(s_stdin_handle, mode) != 0)
    {
        s_console_mode_saved = true;
        (void)atexit(_eos_esh_vscode_restore_terminal);
    }
#else
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
#endif
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

static void _eos_esh_vscode_submit_input(const uint8_t *data, size_t length)
{
    if (length > 0U)
    {
        (void)esh_input(&s_esh, s_owner, data, length);
    }
}

#ifdef _WIN32
static void _eos_esh_vscode_poll_windows(void)
{
    DWORD file_type;

    if (s_stdin_handle == NULL || s_stdin_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    file_type = GetFileType(s_stdin_handle);
    if (file_type == FILE_TYPE_PIPE)
    {
        for (;;)
        {
            DWORD available = 0U;
            DWORD length = 0U;

            if (PeekNamedPipe(s_stdin_handle, NULL, 0U, NULL, &available, NULL) == 0)
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    s_stdin_closed = true;
                }
                return;
            }
            if (available == 0U)
            {
                return;
            }

            if (available > (DWORD)sizeof(s_input_buffer))
            {
                available = (DWORD)sizeof(s_input_buffer);
            }
            if (ReadFile(s_stdin_handle, s_input_buffer, available, &length, NULL) == 0)
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    s_stdin_closed = true;
                }
                return;
            }
            _eos_esh_vscode_submit_input(s_input_buffer, (size_t)length);
            if (length == 0U)
            {
                return;
            }
        }
    }

    if (file_type == FILE_TYPE_CHAR)
    {
        while (_kbhit() != 0)
        {
            int value = _getch();
            if (value == EOF)
            {
                s_stdin_closed = true;
                return;
            }
            s_input_buffer[0] = (uint8_t)value;
            _eos_esh_vscode_submit_input(s_input_buffer, 1U);
        }
    }
}
#endif
#endif

eos_result_t eos_esh_vscode_init(void)
{
#if defined(__EMSCRIPTEN__)
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
#ifdef _WIN32
    if (s_stdin_handle == INVALID_HANDLE_VALUE || s_stdin_handle == NULL)
    {
        s_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    }
#endif
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
#if defined(__EMSCRIPTEN__)
    return;
#else
    if (!s_initialized || s_stdin_closed)
    {
        return;
    }

    esh_poll(&s_esh);
#ifdef _WIN32
    _eos_esh_vscode_poll_windows();
#else
    {
        struct pollfd descriptor = {
            .fd = STDIN_FILENO,
            .events = POLLIN,
            .revents = 0,
        };

        while (poll(&descriptor, 1, 0) > 0 && (descriptor.revents & (POLLIN | POLLHUP)) != 0)
        {
            ssize_t length = read(STDIN_FILENO, s_input_buffer, sizeof(s_input_buffer));
            if (length > 0)
            {
                _eos_esh_vscode_submit_input(s_input_buffer, (size_t)length);
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
    }
#endif
#endif
}
