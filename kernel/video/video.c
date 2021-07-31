#include "video.h"

int cursor_x = 0, cursor_y = 0;

extern PSF fb_font;

uint8_t *fb_addr;
size_t fb_pitch, fb_bpp;
uint16_t width_s, height_s;

void vid_reset(void) {
	 cursor_x = 0;
	 cursor_y = 0;
}

void video_init(struct stivale2_struct_tag_framebuffer *framebuffer){
    fb_addr = (uint8_t *)framebuffer->framebuffer_addr;
	fb_pitch = framebuffer->framebuffer_pitch;
	fb_bpp = framebuffer->framebuffer_bpp;
    width_s = framebuffer->framebuffer_width;
    height_s = framebuffer->framebuffer_height;
}

void draw_px(int x, int y, uint32_t color) {
    size_t py = y * fb_pitch;
    size_t px = x * (fb_bpp / 8);
    *(uint32_t*)(&fb_addr[px + py]) = color;
}


void knewline(void) {
    for (uint32_t y = fb_font.height; y != height_s; ++y) {
      void *dest = (void *)(((uintptr_t)fb_addr) + (y - fb_font.height) * fb_pitch);
      const void *src = (void *)(((uintptr_t)fb_addr) + y * fb_pitch);
      memcpy(dest, src, width_s * 4);
   }
   cursor_y--;

   cursor_x = 131;
   size_t x = (cursor_x * fb_font.width) - fb_font.width, y = cursor_y * fb_font.height;

   size_t i, j;
   while (x >= (2 * fb_font.width)) {
      for (i = 0; i < fb_font.height; i++) {
         for (j = 0; j < fb_font.width; j++) {
            draw_px(x + j, y + i, 0x000000);
         }
      }
      x = (cursor_x * fb_font.width) - fb_font.width, y = cursor_y * fb_font.height;
      cursor_x--;
   }
}

void putchar_at(char c, int position_x, int position_y, uint32_t color, uint32_t bgcolor) {
   switch (c) {
      case '\n':
         cursor_y++;
         cursor_x = 0;
         return;

      case '\r':
         cursor_x = 0;
         return;

      case '\t':
         cursor_x += 5;
         return;

      case '\b':
         cursor_x--;
         return;
   }

   uint8_t *glyph = &fb_font.data[c * fb_font.glyph_size];

   if ((cursor_y * fb_font.height) >= height_s) {
      knewline();
      putchar_at(c, cursor_x, cursor_y, color, bgcolor);
      cursor_x--;
   }

   size_t x = position_x * fb_font.width, y = position_y * fb_font.height;

   static const uint8_t masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};

   size_t i, j;
   for (i = 0; i < fb_font.height; i++) {
      for (j = 0; j < fb_font.width; j++) {
         if (glyph[i] & masks[j]) {
            draw_px(x + j, y + i, color);
         } else {
            draw_px(x + j, y + i, bgcolor);
         }
      }
   }

   if (((cursor_x * fb_font.width) + (2 * fb_font.width)) >= width_s - (2 * fb_font.width)) {
      cursor_y++;
      cursor_x = 0;
   }

   if (c != '\n') {
      cursor_x++;
   }
}

void putchar_color(char c, uint32_t color, uint32_t bgcolor) {
    putchar_at(c, cursor_x, cursor_y, color, bgcolor);
}

void putcharx(char c) {
   putchar_color(c, 0xFFFFFF, 0x000000);
}

void kprint_color(char *string, uint32_t color) {
   while (*string) {
      putchar_at(*string++, cursor_x, cursor_y, color, 0x000000);
   }
}

void kprint(char *string) {
   while (*string) {
      putchar_color(*string++, 0xFFFFFF, 0x000000);
   }
}

void kprintbgc(char *string, uint32_t fcolor, uint32_t bcolor) {
    while(*string) {
        putchar_color(*string++, fcolor, bcolor);
    }
}

void draw_rect(int width, int height, int offx, int offy, uint32_t color) {
    for(int i = 0; i < width + 1; i++){
		for(int j = 0; j < height + 1; j++) {
			draw_px(i + offx, j + offy, color);
		}
	}
}

void clear_screen(uint32_t color){
    uint32_t x, y;
    for(x = 0; x < width_s; x++)
        for(y = 0; y < height_s; y++) {
                size_t py = y * fb_pitch;
                size_t px = x * (fb_bpp / 8);
                *(uint32_t*)(&fb_addr[px + py]) = color;
        }
}
