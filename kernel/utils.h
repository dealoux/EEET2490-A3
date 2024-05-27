#ifndef UTILS_H
#define UTILS_H

#include "../gcclib/stdint.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 1280

// Use RGBA32 (32 bits for each pixel)
#define COLOR_DEPTH 32
// Pixel Order: BGR in memory order (little endian --> RGB in byte order)
#define PIXEL_ORDER 0

void framebfInit();

void drawImage(const unsigned int *image, int x_offset, int y_offset, int width, int height);
void drawRectARGB32(int x1, int y1, int x2, int y2, unsigned int attr, int fill);
void drawPixelARGB32(int x, int y, unsigned int attr);

void drawChar(unsigned char ch, int x, int y, unsigned int attr, int zoom);
void drawString(int x, int y, char *str, unsigned int attr, int zoom);

void drawLineARGB32(int x1, int y1, int x2, int y2, unsigned int color);

unsigned int get_system_time();
void wait_msec(unsigned int msVal);

void set_wait_timer(int set, unsigned int msVal);

int abs(int x);

#endif