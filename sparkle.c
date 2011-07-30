#include <stdlib.h>
#include "serpent.h"

unsigned char pixels[1000];

void next_frame(int frame) {
  int i = rand() % 300;
  if (rand() % 3) {
    pixels[i*3] = 0;
    pixels[i*3 + 1] = 0;
    pixels[i*3 + 2] = 0;
  } else {
    pixels[i*3] = rand() % 256;
    pixels[i*3 + 1] = rand() % 256;
    pixels[i*3 + 2] = rand() % 256;
  }
  for (int s = 0; s < 10; s++) {
    put_pixels(s, pixels, 300);
  }
}

