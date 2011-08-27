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
#define IMPULSE_START 100
#define BUTTON_FORCE 10

#define TICKS_PER_FRAME 10 
#define FPS 20
#define MASS 0.1  // kg
#define FRICTION_FORCE 0.2  // kg*rev^2/s
#define FRICTION_MIN_VELOCITY 0.01  // rev/s

unsigned char pixels[NUM_PIXELS*3];
float position[4][NUM_ROWS];  // rev
float velocity[4][NUM_ROWS];  // rev/s
float impulse[4] = {0, 0, 0, 0};
float spring_constant[4] = {100, 120, 140, 160};
float restore_factor[4] = {0.1, 0.1, 0.1, 0.1};

void tick(float dt) {
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < NUM_ROWS; i++) {
      float force;
      if (i == 0) {
        force = impulse[j];
      } else {
        force = spring_constant[j] * (position[j][i-1] - position[j][i]);
      }
      if (i < NUM_ROWS - 1) {
        force += spring_constant[j] * (position[j][i+1] - position[j][i]);
      }
      force -= restore_factor[j] * position[j][i];
      if (velocity[j][i] > FRICTION_MIN_VELOCITY) {
        force -= FRICTION_FORCE;
      } else if (velocity[j][i] < -FRICTION_MIN_VELOCITY) {
        force += FRICTION_FORCE;
      }
      velocity[j][i] += force/MASS * dt;
    }
    for (int i = 0; i < NUM_ROWS; i++) {
      position[j][i] += velocity[j][i] * dt;
    }
  }
}

static int clock_delay = 0;

void next_frame(int x) {
  if (x == 0) {
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < NUM_ROWS; i++) {
        position[j][i] = 0;
        velocity[j][i] = 0;
      }
    }
  }

  float current_hue = sin(x * 0.01)*0.05 + 0.05;
  int r = 0, g = 0, b = 0;
  int k = (current_hue - floor(current_hue)) * 255 * 3;
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

  impulse[0] = (read_button('b') - read_button('a'))*r;
  impulse[1] = (read_button('b') - read_button('a'))*g;
  impulse[2] = (read_button('b') - read_button('a'))*b;
  impulse[3] = (read_button('y') - read_button('x'))*50;

  for (int t = 0; t < TICKS_PER_FRAME; t++) {
    tick(1.0/FPS/TICKS_PER_FRAME);
  }

  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      int r = 16 + position[0][i]*position[0][i]*20;
      int g = 16 + position[1][i]*position[1][i]*20;
      int b = 16 + position[2][i]*position[2][i]*20;
      r = r < 1 ? 1 : r > 255 ? 255 : r;
      g = g < 1 ? 1 : g > 255 ? 255 : g;
      b = b < 1 ? 1 : b > 255 ? 255 : b;
      set_rgb(pixels, pixel_index(i, j), r, g, b);
//      set_from_palette(pixels, pixel_index(i, j), PALETTE,
//                       position[0][i] + (float) j / NUM_COLUMNS + 0.5);
    }
  }

  for (int s = 0; s < 10; s++) {
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
}
