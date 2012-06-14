#include "serpent.h"

typedef struct {
  byte width;
  byte height;
  byte min_char;
  byte num_chars;
  byte* data;
} font;

font* font_read(char* filename, int min_char, int char_width);

void font_delete(font* f);

typedef void put_pixel_func(int x, int y, byte r, byte g, byte b);

void font_draw(font* f, char* str, int x, int y,
               put_pixel_func* put_pixel, byte r, byte g, byte b);
