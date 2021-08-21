#ifndef VIDEO_H
#define VIDEO_H

/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../klibc/mem.h"
#include "../klibc/string.h"
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

void vid_reset(void);
void video_init(struct stivale2_struct_tag_framebuffer *framebuffer);
void putchar_color(int c, uint32_t color, uint32_t bgcolor);
void putcharx(int c);
void kprint_color(char *string, uint32_t color);
void kprint(char *string);
void kprintbgc(char *string, uint32_t fcolor, uint32_t bcolor);
void draw_rect(int x, int y, int offx, int offy, uint32_t color);
void clear_screen(uint32_t color);

#endif
