// "meteor", by Ka-Ping Yee

#include "serpent.h"

byte pixels[NUM_PIXELS*3];

int modulo(int div, int mod) {
  while (div < 0) div += mod;
  return div % mod;
}

void draw_ball(float crow, float ccol, float radius,
               byte red, byte green, byte blue) {
  for (int r = floor(crow) - radius - 1; r <= ceil(crow) + radius + 1; r++) {
    for (int c = floor(ccol) - radius - 1; c <= ceil(ccol) + radius + 1; c++) {
      if (r >= 0 && r < NUM_ROWS) {
        float d = sqrt((r - crow)*(r - crow) + (c - ccol)*(c - ccol));
        int index = pixel_index(r, modulo(c, NUM_COLUMNS));
        if (d < radius - 0.5) {
          set_rgb(pixels, index, red, green, blue);
        } else if (d < radius + 0.5) {
          float frac = d - (radius - 0.5);
          set_rgb(pixels, index, red*frac, green*frac, blue*frac);
        }
      }
    }
  }
}

void erase_ball(float crow, float ccol, float radius) {
  for (int r = floor(crow) - radius - 1; r <= ceil(crow) + radius + 1; r++) {
    for (int c = floor(ccol) - radius - 1; c <= ceil(ccol) + radius + 1; c++) {
      if (r >= 0 && r < NUM_ROWS) {
        int index = pixel_index(r, modulo(c, NUM_COLUMNS));
        set_rgb(pixels, index, 0, 0, 0);
      }
    }
  }
}

void next_frame(int f) {
  if (f == 0) {
    for (int r = 0; r < NUM_ROWS; r++) {
      for (int c = 0; c < NUM_COLUMNS; c++) {
        set_rgb(pixels, pixel_index(r, c), (r+c)%2, (r+c)%2, 0);
      }
    }
  }

  float x = f; //sin(f/100.0)*50 + 60;
  float y = f; //sin(f/100.0)*50 + 60;

  draw_ball(x, y, 0.5, 240, 120, 0);
  for (int s = 0; s < NUM_SEGS; s++) {
    put_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
  erase_ball(x, y, 3);
}
