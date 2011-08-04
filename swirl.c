// "swirl", by Ka-Ping Yee

// Copyright 2011 Google Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <math.h>
#include "serpent.h"
#include "spectrum.pal"
#include "sunset.pal"
#define PALETTE SPECTRUM_PALETTE

#define SINE_WAVE_AMPLITUDE 0.9
#define SINE_WAVE_PERIOD 40
#define DUTY_CYCLE_ON 120
#define DUTY_CYCLE_OFF 120

#define TICKS_PER_FRAME 10 
#define FPS 20
#define SPRING_CONSTANT 400  // kg/s^2
#define MASS 0.1  // kg
#define FRICTION_FORCE 0.02  // kg*rev^2/s
#define FRICTION_MIN_VELOCITY 0.01  // rev/s

unsigned char pixels[NUM_PIXELS*3];
float position[NUM_ROWS];  // rev
float velocity[NUM_ROWS];  // rev/s

void set_hue(int row, int col, float hue) {
  set_from_palette(pixels, pixel_index(row, col), PALETTE, hue - floor(hue));
}

void tick(float dt) {
  for (int i = 1; i < NUM_ROWS; i++) {
    float force = SPRING_CONSTANT * (position[i-1] - position[i]);
    if (i < NUM_ROWS - 1) {
      force += SPRING_CONSTANT * (position[i+1] - position[i]);
    }
    if (velocity[i] > FRICTION_MIN_VELOCITY) {
      force -= FRICTION_FORCE;
    } else if (velocity[i] < -FRICTION_MIN_VELOCITY) {
      force += FRICTION_FORCE;
    }
    velocity[i] += force/MASS * dt;
  }
  for (int i = 1; i < NUM_ROWS; i++) {
    position[i] += velocity[i] * dt;
  }
}

void next_frame(int x) {
  if (x == 0) {
    for (int i = 0; i < NUM_ROWS; i++) {
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

  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      set_hue(i, j, position[i] + (float) j / NUM_COLUMNS);
    }
  }

  for (int s = 0; s < 10; s++) {
    put_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
}
