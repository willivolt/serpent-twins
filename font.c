#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"

font* font_read(char* filename, int min_char, int char_width) {
  FILE* fp = fopen(filename, "r");
  char line[10];
  int width, height, depth;
  fscanf(fp, "%s\n", line);
  fprintf(stderr, "[%s]\n", line);
  if (strcmp(line, "P5") != 0) {
    fclose(fp);
    return NULL;
  }
  fscanf(fp, "%d\n", &width);
  fprintf(stderr, "[%d]\n", width);
  fscanf(fp, "%d\n", &height);
  fprintf(stderr, "[%d]\n", height);
  fscanf(fp, "%d\n", &depth);
  fprintf(stderr, "[%d]\n", depth);
  if (depth != 255) {
    fclose(fp);
    return NULL;
  }
  font* f = (font*) malloc(sizeof(font));
  f->width = char_width;
  f->height = height;
  f->min_char = min_char;
  f->num_chars = width / char_width;
  f->data = malloc(width * height);
  fread(f->data, width * height, 1, fp);
  fclose(fp);
  return f;
}

void font_delete(font* f) {
  free(f->data);
  free(f);
}

void font_draw(font* f, char* str, int x, int y, put_pixel_func* put_pixel, byte r, byte g, byte b) {
  char* c;
  int dx, dy;
  for (c = str; *c; c++) {
    for (dx = 0; dx < f->width; dx++) {
      for (dy = 0; dy < f->height; dy++) {
        int n = (*c - f->min_char);
        if (n >= 0 && n < f->num_chars) {
          unsigned int k = f->data[n * f->width + dx +
                                   (f->width * f->num_chars)*dy];
          unsigned int rr = r * k, gg = g * k, bb = b * k;
          put_pixel(x + dx, y + dy, rr >> 8, gg >> 8, bb >> 8);
        }
      }
    }
    x += f->width;
  }
}
