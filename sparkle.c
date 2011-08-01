#include <stdlib.h>
#include "serpent.h"

unsigned char pixels[1000];

void next_frame(int frame) {
  int i = rand() % 300;
  if (rand() % 3) {
    set_rgb(pixels, i, 0, 0, 0);
  } else {
    set_rgb(pixels, i, rand(), rand(), rand());
  }
  for (int s = 0; s < 10; s++) {
    put_pixels(s, pixels, 300);
  }
}

