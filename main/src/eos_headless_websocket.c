/**
 * @file eos_headless_websocket.c
 * @brief Small dependency-free RFC6455 server for direct Webview streaming.
 *
 * The server intentionally supports one loopback client.  Frame data is sent
 * as one unmasked binary WebSocket message.  The server waits for the
 * Webview's "presented" control message before sending the next frame; while
 * a frame is in flight, a new frame replaces the one pending for delivery.
 */

#include "eos_headless_websocket.h"

#if !defined(__EMSCRIPTEN__)

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <strings.h>
#else
#define strncasecmp _strnicmp
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
typedef SOCKET eos_ws_socket_t;
#define EOS_WS_INVALID_SOCKET INVALID_SOCKET
#define eos_ws_close_socket closesocket
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int eos_ws_socket_t;
#define EOS_WS_INVALID_SOCKET (-1)
#define eos_ws_close_socket close
#endif

#define EOS_WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define EOS_WS_RX_SIZE 8192U
#define EOS_WS_HANDSHAKE_SIZE 2048U
#define EOS_WS_CONTROL_SIZE 256U
#define EOS_WS_TOKEN_SIZE 33U
#define EOS_WS_ENDPOINT_SIZE 256U
#define EOS_WS_FRAME_TIMEOUT_MS 5000U

static eos_ws_socket_t s_listener = EOS_WS_INVALID_SOCKET;
static eos_ws_socket_t s_client = EOS_WS_INVALID_SOCKET;
static bool s_initialized;
static bool s_handshake_complete;
static bool s_frame_in_flight;
static bool s_frame_dirty;
static bool s_has_frame;
static uint32_t s_width;
static uint32_t s_height;
static size_t s_frame_size;
static uint8_t *s_frame_buffers[2];
static uint8_t s_active_frame;
static uint8_t s_rx_buffer[EOS_WS_RX_SIZE];
static size_t s_rx_size;
static uint8_t s_handshake_response[EOS_WS_HANDSHAKE_SIZE];
static size_t s_handshake_response_size;
static size_t s_handshake_response_offset;
static uint8_t s_frame_header[10];
static size_t s_frame_header_offset;
static size_t s_frame_offset;
static uint64_t s_frame_started_ms;
static char s_token[EOS_WS_TOKEN_SIZE];
static char s_endpoint[EOS_WS_ENDPOINT_SIZE];
static eos_headless_websocket_input_callback_t s_input_callback;
static void *s_input_user_data;

static uint64_t _monotonic_ms(void)
{
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0U;
    return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
#endif
}

static bool _socket_would_block(void)
{
#ifdef _WIN32
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINTR;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
#endif
}

static bool _set_nonblocking(eos_ws_socket_t socket_handle)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(socket_handle, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket_handle, F_GETFL, 0);
    return flags >= 0 && fcntl(socket_handle, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

static void _close_client(void)
{
    if (!s_frame_dirty && s_has_frame && s_frame_buffers[0] && s_frame_buffers[1])
    {
        /* The active frame is the latest complete frame whenever there is no
         * newer pending frame. Copy it to the pending slot so a replacement
         * client receives the current image even if the old client disconnected
         * after acknowledging its last frame. */
        memcpy(s_frame_buffers[1U - s_active_frame], s_frame_buffers[s_active_frame], s_frame_size);
    }
    if (s_client != EOS_WS_INVALID_SOCKET)
    {
        eos_ws_close_socket(s_client);
        s_client = EOS_WS_INVALID_SOCKET;
    }
    s_handshake_complete = false;
    s_frame_in_flight = false;
    s_frame_dirty = s_has_frame;
    s_rx_size = 0U;
    s_handshake_response_size = 0U;
    s_handshake_response_offset = 0U;
    s_frame_header_offset = 0U;
    s_frame_offset = 0U;
    s_frame_started_ms = 0U;
}

static uint32_t _rotate_left(uint32_t value, uint32_t bits)
{
    return (value << bits) | (value >> (32U - bits));
}

static void _sha1_transform(uint32_t state[5], const uint8_t block[64])
{
    uint32_t words[80];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;

    for (uint32_t index = 0U; index < 16U; index++)
    {
        words[index] = ((uint32_t)block[index * 4U] << 24U) | ((uint32_t)block[index * 4U + 1U] << 16U)
                       | ((uint32_t)block[index * 4U + 2U] << 8U) | block[index * 4U + 3U];
    }
    for (uint32_t index = 16U; index < 80U; index++)
    {
        words[index] =
            _rotate_left(words[index - 3U] ^ words[index - 8U] ^ words[index - 14U] ^ words[index - 16U], 1U);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    for (uint32_t index = 0U; index < 80U; index++)
    {
        uint32_t function;
        uint32_t constant;
        uint32_t temporary;
        if (index < 20U)
        {
            function = (b & c) | ((~b) & d);
            constant = 0x5A827999U;
        }
        else if (index < 40U)
        {
            function = b ^ c ^ d;
            constant = 0x6ED9EBA1U;
        }
        else if (index < 60U)
        {
            function = (b & c) | (b & d) | (c & d);
            constant = 0x8F1BBCDCU;
        }
        else
        {
            function = b ^ c ^ d;
            constant = 0xCA62C1D6U;
        }
        temporary = _rotate_left(a, 5U) + function + e + constant + words[index];
        e = d;
        d = c;
        c = _rotate_left(b, 30U);
        b = a;
        a = temporary;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void _sha1(const uint8_t *input, size_t input_size, uint8_t output[20])
{
    uint32_t state[5] = {0x67452301U, 0xEFCDAB89U, 0x98BADCFEU, 0x10325476U, 0xC3D2E1F0U};
    uint8_t block[64];
    uint64_t bit_count = (uint64_t)input_size * 8U;
    size_t offset = 0U;

    while (input_size - offset >= 64U)
    {
        _sha1_transform(state, input + offset);
        offset += 64U;
    }
    memset(block, 0, sizeof(block));
    memcpy(block, input + offset, input_size - offset);
    block[input_size - offset] = 0x80U;
    if (input_size - offset >= 56U)
    {
        _sha1_transform(state, block);
        memset(block, 0, sizeof(block));
    }
    for (uint32_t index = 0U; index < 8U; index++)
    {
        block[63U - index] = (uint8_t)(bit_count >> (index * 8U));
    }
    _sha1_transform(state, block);
    for (uint32_t index = 0U; index < 5U; index++)
    {
        output[index * 4U] = (uint8_t)(state[index] >> 24U);
        output[index * 4U + 1U] = (uint8_t)(state[index] >> 16U);
        output[index * 4U + 2U] = (uint8_t)(state[index] >> 8U);
        output[index * 4U + 3U] = (uint8_t)state[index];
    }
}

static size_t _base64(const uint8_t *input, size_t input_size, char *output, size_t output_size)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_offset = 0U;
    for (size_t index = 0U; index < input_size; index += 3U)
    {
        uint32_t value = (uint32_t)input[index] << 16U;
        size_t remaining = input_size - index;
        if (remaining > 1U)
            value |= (uint32_t)input[index + 1U] << 8U;
        if (remaining > 2U)
            value |= input[index + 2U];
        if (output_offset + 4U >= output_size)
            return 0U;
        output[output_offset++] = alphabet[(value >> 18U) & 0x3FU];
        output[output_offset++] = alphabet[(value >> 12U) & 0x3FU];
        output[output_offset++] = remaining > 1U ? alphabet[(value >> 6U) & 0x3FU] : '=';
        output[output_offset++] = remaining > 2U ? alphabet[value & 0x3FU] : '=';
    }
    output[output_offset] = '\0';
    return output_offset;
}

static void _make_token(void)
{
    static const char hex[] = "0123456789abcdef";
    unsigned int seed = (unsigned int)time(NULL);
#ifdef _WIN32
    seed ^= (unsigned int)_getpid();
#else
    seed ^= (unsigned int)getpid();
#endif
    srand(seed);
    for (size_t index = 0U; index < EOS_WS_TOKEN_SIZE - 1U; index++)
    {
        s_token[index] = hex[rand() & 0x0FU];
    }
    s_token[EOS_WS_TOKEN_SIZE - 1U] = '\0';
}

static const char *_find_header(const char *request, const char *name)
{
    size_t name_size = strlen(name);
    for (const char *cursor = request; *cursor != '\0'; cursor++)
    {
        if ((cursor == request || cursor[-1] == '\n') && strncasecmp(cursor, name, name_size) == 0)
        {
            cursor += name_size;
            while (*cursor == ' ' || *cursor == '\t')
                cursor++;
            return cursor;
        }
    }
    return NULL;
}

static bool _token_matches(const char *request)
{
    const char *token = strstr(request, "token=");
    size_t token_size;
    if (!token)
        return false;
    token += 6U;
    token_size = strcspn(token, "& \r\n");
    return token_size == strlen(s_token) && strncmp(token, s_token, token_size) == 0;
}

static bool _prepare_handshake(void)
{
    char request[EOS_WS_HANDSHAKE_SIZE];
    char accept_input[256];
    char accept_text[64];
    uint8_t digest[20];
    const char *key;
    const char *end = NULL;
    size_t request_size;
    size_t key_size;
    int length;

    for (size_t index = 3U; index < s_rx_size; index++)
    {
        if (s_rx_buffer[index - 3U] == '\r' && s_rx_buffer[index - 2U] == '\n' && s_rx_buffer[index - 1U] == '\r'
            && s_rx_buffer[index] == '\n')
        {
            end = (const char *)(s_rx_buffer + index + 1U);
            break;
        }
    }
    if (!end)
        return true;
    request_size = (size_t)(end - (const char *)s_rx_buffer);
    if (request_size >= sizeof(request))
        return false;
    memcpy(request, s_rx_buffer, request_size);
    request[request_size] = '\0';
    if (strncmp(request, "GET ", 4U) != 0 || !_token_matches(request))
        return false;
    key = _find_header(request, "Sec-WebSocket-Key:");
    if (!key)
        return false;
    key_size = strcspn(key, "\r\n");
    if (key_size == 0U || key_size >= sizeof(accept_input) - sizeof(EOS_WS_GUID))
        return false;
    length = snprintf(accept_input, sizeof(accept_input), "%.*s%s", (int)key_size, key, EOS_WS_GUID);
    if (length <= 0 || (size_t)length >= sizeof(accept_input))
        return false;
    _sha1((const uint8_t *)accept_input, (size_t)length, digest);
    if (_base64(digest, sizeof(digest), accept_text, sizeof(accept_text)) == 0U)
        return false;
    length = snprintf((char *)s_handshake_response,
                      sizeof(s_handshake_response),
                      "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: %s\r\n\r\n",
                      accept_text);
    if (length <= 0 || (size_t)length >= sizeof(s_handshake_response))
        return false;
    s_handshake_response_size = (size_t)length;
    s_handshake_response_offset = 0U;
    s_rx_size -= request_size;
    if (s_rx_size > 0U)
        memmove(s_rx_buffer, s_rx_buffer + request_size, s_rx_size);
    s_handshake_complete = true;
    return true;
}

static bool _send_bytes(const uint8_t *buffer, size_t size, size_t *offset)
{
    while (*offset < size)
    {
#ifdef _WIN32
        int written = send(s_client, (const char *)buffer + *offset, (int)(size - *offset), 0);
#else
        ssize_t written = send(s_client, buffer + *offset, size - *offset, MSG_NOSIGNAL);
#endif
        if (written > 0)
        {
            *offset += (size_t)written;
            continue;
        }
        if (written < 0 && _socket_would_block())
            return true;
        return false;
    }
    return true;
}

static bool _read_socket(void)
{
    for (;;)
    {
#ifdef _WIN32
        int received = recv(s_client, (char *)s_rx_buffer + s_rx_size, (int)(sizeof(s_rx_buffer) - s_rx_size), 0);
#else
        ssize_t received = recv(s_client, s_rx_buffer + s_rx_size, sizeof(s_rx_buffer) - s_rx_size, 0);
#endif
        if (received > 0)
        {
            s_rx_size += (size_t)received;
            continue;
        }
        if (received == 0)
            return false;
        if (_socket_would_block())
            return true;
        return false;
    }
}

static void _handle_text(const uint8_t *payload, size_t payload_size)
{
    char message[EOS_WS_CONTROL_SIZE];
    char action[8];
    char *number_end;
    char *x_text;
    char *y_text;
    char *action_end;
    long x;
    long y;
    size_t copy_size = payload_size < sizeof(message) - 1U ? payload_size : sizeof(message) - 1U;

    memcpy(message, payload, copy_size);
    message[copy_size] = '\0';
    if (strstr(message, "presented") != NULL)
    {
        s_frame_in_flight = false;
        s_frame_started_ms = 0U;
        return;
    }
    if (strstr(message, "\"type\":\"button\"") != NULL)
    {
        eos_headless_websocket_input_action_t button_action;
        if (strstr(message, "\"button\":\"crown\"") != NULL && strstr(message, "\"action\":\"longPress\"") != NULL)
        {
            button_action = EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_LONG_PRESS;
        }
        else if (strstr(message, "\"button\":\"crown\"") != NULL)
        {
            button_action = EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_CLICK;
        }
        else if (strstr(message, "\"button\":\"side\"") != NULL)
        {
            button_action = EOS_HEADLESS_WEBSOCKET_BUTTON_SIDE_CLICK;
        }
        else
        {
            return;
        }
        if (s_input_callback)
            s_input_callback(button_action, 0, 0, s_input_user_data);
        return;
    }
    if (strstr(message, "\"type\":\"input\"") == NULL)
        return;
    char *action_text = strstr(message, "\"action\":\"");
    x_text = strstr(message, "\"x\":");
    y_text = strstr(message, "\"y\":");
    if (!action_text || !x_text || !y_text)
        return;
    action_text += strlen("\"action\":\"");
    action_end = strchr(action_text, '\"');
    if (!action_end || action_end == action_text)
        return;
    size_t action_size = (size_t)(action_end - action_text);
    if (action_size >= sizeof(action))
        return;
    memcpy(action, action_text, action_size);
    action[action_size] = '\0';
    x = strtol(x_text + 4U, &number_end, 10);
    if (number_end == x_text + 4U)
        return;
    y = strtol(y_text + 4U, &number_end, 10);
    if (number_end == y_text + 4U)
        return;
    if (s_input_callback)
    {
        eos_headless_websocket_input_action_t input_action;
        if (strcmp(action, "down") == 0)
            input_action = EOS_HEADLESS_WEBSOCKET_INPUT_DOWN;
        else if (strcmp(action, "move") == 0)
            input_action = EOS_HEADLESS_WEBSOCKET_INPUT_MOVE;
        else if (strcmp(action, "up") == 0)
            input_action = EOS_HEADLESS_WEBSOCKET_INPUT_UP;
        else
            return;
        s_input_callback(input_action, (int32_t)x, (int32_t)y, s_input_user_data);
    }
}

static bool _parse_websocket_frames(void)
{
    while (s_rx_size >= 2U)
    {
        uint8_t first = s_rx_buffer[0];
        uint8_t second = s_rx_buffer[1];
        uint64_t payload_size = second & 0x7FU;
        size_t header_size = 2U;
        uint8_t mask[4];
        size_t frame_size;

        if ((first & 0x80U) == 0U)
            return false;
        if ((second & 0x80U) == 0U)
            return false;
        if (payload_size == 126U)
        {
            if (s_rx_size < 4U)
                return true;
            payload_size = ((uint64_t)s_rx_buffer[2] << 8U) | s_rx_buffer[3];
            header_size = 4U;
        }
        else if (payload_size == 127U)
        {
            if (s_rx_size < 10U)
                return true;
            payload_size = 0U;
            for (size_t index = 0U; index < 8U; index++)
                payload_size = (payload_size << 8U) | s_rx_buffer[2U + index];
            header_size = 10U;
        }
        if (payload_size > EOS_WS_CONTROL_SIZE || s_rx_size < header_size + 4U)
            return false;
        memcpy(mask, s_rx_buffer + header_size, sizeof(mask));
        header_size += 4U;
        frame_size = header_size + (size_t)payload_size;
        if (s_rx_size < frame_size)
            return true;
        for (size_t index = 0U; index < (size_t)payload_size; index++)
            s_rx_buffer[header_size + index] ^= mask[index & 3U];
        switch (first & 0x0FU)
        {
            case 0x1U:
                _handle_text(s_rx_buffer + header_size, (size_t)payload_size);
                break;
            case 0x8U:
                return false;
            case 0x9U:
                break; /* Browser control traffic is optional for this local stream. */
            default:
                break;
        }
        s_rx_size -= frame_size;
        if (s_rx_size > 0U)
            memmove(s_rx_buffer, s_rx_buffer + frame_size, s_rx_size);
    }
    return true;
}

static void _start_next_frame(void)
{
    uint64_t size = (uint64_t)s_frame_size;
    if (!s_handshake_complete || s_frame_in_flight || !s_frame_dirty)
        return;
    s_active_frame ^= 1U;
    s_frame_dirty = false;
    s_frame_header[0] = 0x82U;
    s_frame_header[1] = 127U;
    for (size_t index = 0U; index < 8U; index++)
        s_frame_header[9U - index] = (uint8_t)(size >> (index * 8U));
    s_frame_header_offset = 0U;
    s_frame_offset = 0U;
    s_frame_in_flight = true;
    s_frame_started_ms = _monotonic_ms();
}

static bool _flush_output(void)
{
    if (!s_handshake_complete)
        return true;
    if (s_handshake_response_offset < s_handshake_response_size
        && !_send_bytes(s_handshake_response, s_handshake_response_size, &s_handshake_response_offset))
        return false;
    if (s_handshake_response_offset < s_handshake_response_size)
        return true;
    _start_next_frame();
    if (!s_frame_in_flight)
        return true;
    if (s_frame_header_offset < sizeof(s_frame_header)
        && !_send_bytes(s_frame_header, sizeof(s_frame_header), &s_frame_header_offset))
        return false;
    if (s_frame_header_offset < sizeof(s_frame_header))
        return true;
    if (s_frame_offset < s_frame_size && !_send_bytes(s_frame_buffers[s_active_frame], s_frame_size, &s_frame_offset))
        return false;
    return true;
}

static void _accept_client(void)
{
    struct sockaddr_in address;
#ifdef _WIN32
    int address_size = sizeof(address);
#else
    socklen_t address_size = sizeof(address);
#endif
    eos_ws_socket_t client = accept(s_listener, (struct sockaddr *)&address, &address_size);
    if (client == EOS_WS_INVALID_SOCKET)
        return;
    if (!_set_nonblocking(client))
    {
        eos_ws_close_socket(client);
        return;
    }
    _close_client();
    s_client = client;
}

bool eos_headless_websocket_init(uint16_t requested_port,
                                 uint32_t width,
                                 uint32_t height,
                                 eos_headless_websocket_input_callback_t input_callback,
                                 void *input_user_data)
{
    struct sockaddr_in address;
    int reuse = 1;
#ifdef _WIN32
    int address_size = sizeof(address);
#else
    socklen_t address_size = sizeof(address);
#endif
    if (s_initialized)
        return true;
    s_width = width;
    s_height = height;
    s_input_callback = input_callback;
    s_input_user_data = input_user_data;
    s_frame_size = (size_t)width * (size_t)height * 2U;
    s_frame_buffers[0] = (uint8_t *)calloc(1U, s_frame_size);
    s_frame_buffers[1] = (uint8_t *)calloc(1U, s_frame_size);
    if (!s_frame_buffers[0] || !s_frame_buffers[1])
        goto fail;
    s_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_listener == EOS_WS_INVALID_SOCKET || !_set_nonblocking(s_listener))
        goto fail;
    (void)setsockopt(s_listener, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(requested_port);
    if (bind(s_listener, (struct sockaddr *)&address, sizeof(address)) != 0 || listen(s_listener, 1) != 0
        || getsockname(s_listener, (struct sockaddr *)&address, &address_size) != 0)
        goto fail;
    _make_token();
    (void)snprintf(s_endpoint,
                   sizeof(s_endpoint),
                   "ws://127.0.0.1:%u/?token=%s",
                   (unsigned)ntohs(address.sin_port),
                   s_token);
    s_initialized = true;
    return true;

fail:
    eos_headless_websocket_shutdown();
    return false;
}

void eos_headless_websocket_poll(void)
{
    if (!s_initialized)
        return;
    if (s_client == EOS_WS_INVALID_SOCKET)
        _accept_client();
    if (s_client == EOS_WS_INVALID_SOCKET)
        return;
    if (!_read_socket())
    {
        _close_client();
        return;
    }
    if (s_frame_in_flight && s_frame_started_ms != 0U && _monotonic_ms() - s_frame_started_ms > EOS_WS_FRAME_TIMEOUT_MS)
    {
        /* A hidden/suspended Webview can leave a frame awaiting its
         * "presented" acknowledgement forever. Recycle only that client;
         * the Simulator and LVGL state remain intact and the Webview can
         * reconnect to receive the retained latest frame. */
        _close_client();
        return;
    }
    if (!s_handshake_complete)
    {
        if (s_rx_size >= sizeof(s_rx_buffer) || !_prepare_handshake())
        {
            _close_client();
            return;
        }
    }
    else if (!_parse_websocket_frames())
    {
        _close_client();
        return;
    }
    if (!_flush_output())
        _close_client();
}

bool eos_headless_websocket_submit_frame(const uint8_t *rgb565, size_t size)
{
    if (!s_initialized || !rgb565 || size != s_frame_size)
        return false;
    s_has_frame = true;
    memcpy(s_frame_buffers[1U - s_active_frame], rgb565, s_frame_size);
    s_frame_dirty = true;
    return true;
}

const char *eos_headless_websocket_endpoint(void)
{
    return s_initialized ? s_endpoint : NULL;
}

void eos_headless_websocket_shutdown(void)
{
    _close_client();
    if (s_listener != EOS_WS_INVALID_SOCKET)
    {
        eos_ws_close_socket(s_listener);
        s_listener = EOS_WS_INVALID_SOCKET;
    }
    free(s_frame_buffers[0]);
    free(s_frame_buffers[1]);
    s_frame_buffers[0] = NULL;
    s_frame_buffers[1] = NULL;
    s_frame_size = 0U;
    s_has_frame = false;
    s_input_callback = NULL;
    s_input_user_data = NULL;
    s_initialized = false;
    s_endpoint[0] = '\0';
}

#else

bool eos_headless_websocket_init(uint16_t requested_port,
                                 uint32_t width,
                                 uint32_t height,
                                 eos_headless_websocket_input_callback_t input_callback,
                                 void *input_user_data)
{
    (void)requested_port;
    (void)width;
    (void)height;
    (void)input_callback;
    (void)input_user_data;
    return false;
}

void eos_headless_websocket_poll(void)
{
}

bool eos_headless_websocket_submit_frame(const uint8_t *rgb565, size_t size)
{
    (void)rgb565;
    (void)size;
    return false;
}

const char *eos_headless_websocket_endpoint(void)
{
    return NULL;
}

void eos_headless_websocket_shutdown(void)
{
}

#endif
