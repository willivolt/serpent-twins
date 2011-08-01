#include <math.h>
#include "serpent.h"

#define CIRCUMFERENCE 25
#define LENGTH 120
#define PIXELS_PER_SEGMENT (CIRCUMFERENCE*LENGTH/10)
#define SINE_WAVE_AMPLITUDE 0.8
#define SINE_WAVE_PERIOD 80
#define DUTY_CYCLE_ON 120
#define DUTY_CYCLE_OFF 200

#define TICKS_PER_FRAME 10 
#define FPS 20
#define SPRING_CONSTANT 3000  // kg/s^2
#define MASS 0.1  // kg
#define FRICTION_FORCE 0.05  // kg*rev^2/s
#define FRICTION_MIN_VELOCITY 0.01  // rev/s

unsigned char pixels[CIRCUMFERENCE*LENGTH*3];
float position[LENGTH];  // rev
float velocity[LENGTH];  // rev/s

void set_hue(int row, int angle, float hue) {
  unsigned char r = 0, g = 0, b = 0;
  int k = (hue - floor(hue)) * 255 * 3;
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
  set_rgb(pixels, index, r ? r : 1, g ? g : 1, b ? b : 1);
}

void tick(float dt) {
  for (int i = 1; i < LENGTH; i++) {
    float force = SPRING_CONSTANT * (position[i-1] - position[i]);
    if (i < LENGTH - 1) {
      force += SPRING_CONSTANT * (position[i+1] - position[i]);
    }
    if (velocity[i] > FRICTION_MIN_VELOCITY) {
      force -= FRICTION_FORCE;
    } else if (velocity[i] < -FRICTION_MIN_VELOCITY) {
      force += FRICTION_FORCE;
    }
    velocity[i] += force/MASS * dt;
  }
  for (int i = 1; i < LENGTH; i++) {
    position[i] += velocity[i] * dt;
  }
}

void next_frame(int x) {
  if (x == 0) {
    for (int i = 0; i < LENGTH; i++) {
      position[i] = 0;
      velocity[i] = 0;
    }
  }

  float duty_phase = x % (DUTY_CYCLE_ON + DUTY_CYCLE_OFF);
  if (duty_phase < DUTY_CYCLE_ON) {
    position[0] = sin(2*M_PI*(duty_phase/SINE_WAVE_PERIOD))*SINE_WAVE_AMPLITUDE;
  }
  for (int t = 0; t < TICKS_PER_FRAME; t++) {
    tick(1.0/FPS/TICKS_PER_FRAME);
  }

  for (int i = 0; i < LENGTH; i++) {
    for (int j = 0; j < CIRCUMFERENCE; j++) {
      set_hue(i, j, position[i] + (float) j / CIRCUMFERENCE);
    }
  }

  for (int s = 0; s < 10; s++) {
    put_pixels(s, pixels + s*PIXELS_PER_SEGMENT*3, PIXELS_PER_SEGMENT);
  }
}
