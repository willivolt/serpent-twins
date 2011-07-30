#include "serpent.h"

#define CIRCUMFERENCE 25
#define HEIGHT 12
#define SQUARE_WAVE_AMPLITUDE 0.5
#define SQUARE_WAVE_PERIOD 100
#define SPRING_CONSTANT 0.05
#define FRICTION 0.2

#define set_red(index, value) pixels[(index)*3] = value
#define set_green(index, value) pixels[(index)*3 + 1] = value
#define set_blue(index, value) pixels[(index)*3 + 2] = value

unsigned char pixels[CIRCUMFERENCE*HEIGHT*3];
float shift[HEIGHT];
float velocity[HEIGHT];

void set_hue(int row, int angle, float hue) {
  unsigned char r = 0, g = 0, b = 0;
  int k = (hue - (int) hue) * 255 * 3;
  if (k < 255) {
    r = 255 - k;
    g = k;
  } else if (k < 510) {
    g = 255 - (k - 255);
    b = k - 255;
  } else {
    b = 255 - (k - 510);
    r = k - 510;
  }

  int index = (row * CIRCUMFERENCE) +
      ((row % 2) ? angle : CIRCUMFERENCE - 1 - angle);
  set_red(index, r ? r : 1);
  set_green(index, g ? g : 1);
  set_blue(index, b ? b : 1);
}

void next_frame(int x) {
  if (x == 0) {
    for (int i = 0; i < HEIGHT; i++) {
      shift[i] = 0;
      velocity[i] = 0;
    }
  }

  shift[0] = (((int) (x/SQUARE_WAVE_PERIOD)) % 2) * SQUARE_WAVE_AMPLITUDE;
  for (int i = 1; i < HEIGHT; i++) {
    velocity[i] += SPRING_CONSTANT * (shift[i-1] - shift[i]);
  }
  for (int i = 1; i < HEIGHT; i++) {
    velocity[i] *= (1 - FRICTION);
  }
  for (int i = 1; i < HEIGHT; i++) {
    shift[i] += velocity[i];
  }
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < CIRCUMFERENCE; j++) {
      set_hue(i, j, shift[i] + (float) j / CIRCUMFERENCE);
    }
  }
  put_pixels(0, pixels, CIRCUMFERENCE*HEIGHT);
}
