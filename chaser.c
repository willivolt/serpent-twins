#include "serpent.h"

unsigned char pixels[1000];

void next_frame(int frame) {
  int pos = frame % 300;
  for (int i = 0; i < 300; i++) {
    pixels[i*3] = (i == pos) ? 255 : 0;
    pixels[i*3 + 1] = (i == pos + 1) ? 255 : 0;
    pixels[i*3 + 2] = (i == pos + 2) ? 255 : 0;
  }
  put_pixels(0, pixels, 300);
}

