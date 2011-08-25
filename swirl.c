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
#define SPRING_CONSTANT 400  // kg/s^2
#define MASS 0.1  // kg
#define FRICTION_FORCE 0.03  // kg*rev^2/s
#define FRICTION_MIN_VELOCITY 0.01  // rev/s

unsigned char pixels[NUM_PIXELS*3];
float position[NUM_ROWS];  // rev
float velocity[NUM_ROWS];  // rev/s
int auto_impulse = 1;

void set_hue(int row, int col, float hue) {
  set_from_palette(pixels, pixel_index(row, col), PALETTE, hue - floor(hue));
}

void tick(float dt) {
  for (int i = 0; i < NUM_ROWS; i++) {
    float force;
    if (i == 0) {
      if (auto_impulse) {
        continue;
      }
      force = (read_button('b') - read_button('a'))*BUTTON_FORCE;
    } else {
      force = SPRING_CONSTANT * (position[i-1] - position[i]);
    }
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
  for (int i = 0; i < NUM_ROWS; i++) {
    position[i] += velocity[i] * dt;
  }
}

static int clock_delay = 0;

void next_frame(int x) {
  if (x == 0) {
    auto_impulse = 1;
    for (int i = 0; i < NUM_ROWS; i++) {
      position[i] = 0;
      velocity[i] = 0;
    }
  }

  if (auto_impulse) {
    float duty_phase = (x - IMPULSE_START) % (DUTY_CYCLE_ON + DUTY_CYCLE_OFF);
    if (x > IMPULSE_START && duty_phase < DUTY_CYCLE_ON) {
      position[0] = sin(
          2*M_PI*(duty_phase/SINE_WAVE_PERIOD))*SINE_WAVE_AMPLITUDE;
    }
  }
  if (read_button('a') || read_button('b')) {
    auto_impulse = 0;
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
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }

  if (read_button('x')) {
    if (clock_delay < 200) clock_delay++;
    printf("delay %d\n", clock_delay);
    tcl_set_clock_delay(clock_delay);
  }

  if (read_button('y')) {
    if (clock_delay > 0) clock_delay--;
    printf("delay %d\n", clock_delay);
    tcl_set_clock_delay(clock_delay);
  }
}
