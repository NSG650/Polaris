/* Copyright (C) 2022-2024 mintsuki and contributors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLANTERM_H
#define FLANTERM_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FLANTERM_MAX_ESC_VALUES 16

#define FLANTERM_CB_DEC 10
#define FLANTERM_CB_BELL 20
#define FLANTERM_CB_PRIVATE_ID 30
#define FLANTERM_CB_STATUS_REPORT 40
#define FLANTERM_CB_POS_REPORT 50
#define FLANTERM_CB_KBD_LEDS 60
#define FLANTERM_CB_MODE 70
#define FLANTERM_CB_LINUX 80

#define FLANTERM_OOB_OUTPUT_OCRNL (1 << 0)
#define FLANTERM_OOB_OUTPUT_OFDEL (1 << 1)
#define FLANTERM_OOB_OUTPUT_OFILL (1 << 2)
#define FLANTERM_OOB_OUTPUT_OLCUC (1 << 3)
#define FLANTERM_OOB_OUTPUT_ONLCR (1 << 4)
#define FLANTERM_OOB_OUTPUT_ONLRET (1 << 5)
#define FLANTERM_OOB_OUTPUT_ONOCR (1 << 6)
#define FLANTERM_OOB_OUTPUT_OPOST (1 << 7)

struct flanterm_context {
    /* internal use */

    size_t tab_size;
    bool autoflush;
    bool cursor_enabled;
    bool scroll_enabled;
    bool control_sequence;
    bool escape;
    bool osc;
    bool osc_escape;
    bool rrr;
    bool discard_next;
    bool bold;
    bool bg_bold;
    bool reverse_video;
    bool dec_private;
    bool insert_mode;
    uint64_t code_point;
    size_t unicode_remaining;
    uint8_t g_select;
    uint8_t charsets[2];
    size_t current_charset;
    size_t escape_offset;
    size_t esc_values_i;
    size_t saved_cursor_x;
    size_t saved_cursor_y;
    size_t current_primary;
    size_t current_bg;
    size_t scroll_top_margin;
    size_t scroll_bottom_margin;
    uint32_t esc_values[FLANTERM_MAX_ESC_VALUES];
    uint64_t oob_output;
    bool saved_state_bold;
    bool saved_state_bg_bold;
    bool saved_state_reverse_video;
    size_t saved_state_current_charset;
    size_t saved_state_current_primary;
    size_t saved_state_current_bg;

    /* to be set by backend */

    size_t rows, cols;

    void (*raw_putchar)(struct flanterm_context *, uint8_t c);
    void (*clear)(struct flanterm_context *, bool move);
    void (*set_cursor_pos)(struct flanterm_context *, size_t x, size_t y);
    void (*get_cursor_pos)(struct flanterm_context *, size_t *x, size_t *y);
    void (*set_text_fg)(struct flanterm_context *, size_t fg);
    void (*set_text_bg)(struct flanterm_context *, size_t bg);
    void (*set_text_fg_bright)(struct flanterm_context *, size_t fg);
    void (*set_text_bg_bright)(struct flanterm_context *, size_t bg);
    void (*set_text_fg_rgb)(struct flanterm_context *, uint32_t fg);
    void (*set_text_bg_rgb)(struct flanterm_context *, uint32_t bg);
    void (*set_text_fg_default)(struct flanterm_context *);
    void (*set_text_bg_default)(struct flanterm_context *);
    void (*set_text_fg_default_bright)(struct flanterm_context *);
    void (*set_text_bg_default_bright)(struct flanterm_context *);
    void (*move_character)(struct flanterm_context *, size_t new_x, size_t new_y, size_t old_x, size_t old_y);
    void (*scroll)(struct flanterm_context *);
    void (*revscroll)(struct flanterm_context *);
    void (*swap_palette)(struct flanterm_context *);
    void (*save_state)(struct flanterm_context *);
    void (*restore_state)(struct flanterm_context *);
    void (*double_buffer_flush)(struct flanterm_context *);
    void (*full_refresh)(struct flanterm_context *);
    void (*deinit)(struct flanterm_context *, void (*)(void *, size_t));

    /* to be set by client */

    void (*callback)(struct flanterm_context *, uint64_t, uint64_t, uint64_t, uint64_t);
};

void flanterm_context_reinit(struct flanterm_context *ctx);
void flanterm_write(struct flanterm_context *ctx, const char *buf, size_t count);

#ifdef __cplusplus
}
#endif

#endif
