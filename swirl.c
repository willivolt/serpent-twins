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
#include <stdlib.h>
#include "serpent.h"
#include "spectrum.pal"
#include "sunset.pal"
#define PALETTE SPECTRUM_PALETTE

#define DUTY_CYCLE_ON 120
#define DUTY_CYCLE_OFF 120
#define IMPULSE_START 100

#define TICKS_PER_FRAME 10 
#define FPS 20
#define SPRING_CONSTANT 400  // kg/s^2
#define MASS 0.1  // kg
#define FRICTION_MIN_VELOCITY 0.02  // rev/s

unsigned char pixels[NUM_PIXELS*3];
float position[NUM_ROWS];  // rev
float velocity[NUM_ROWS];  // rev/s
int auto_impulse = 1;
float button_force = 30;
float restore_factor = 1;
float restore_center = 0;
float auto_impulse_period = 40;
float auto_impulse_amplitude = 0.9;

void tick(float dt) {
  float friction_force = 0.05 + read_button('y')*0.2;
  float friction_min_velocity = friction_force/MASS * dt;

  for (int i = 0; i < NUM_ROWS; i++) {
    float force;
    if (i == 0) {
      force = accel_right()*0.7 +
          (read_button('b') - read_button('a'))*button_force;
      if (force < -25 || force > 25) {
        auto_impulse = 0;
      }
      if (auto_impulse) {
        continue;
      }
    } else {
      force = SPRING_CONSTANT * (position[i-1] - position[i]);
    }
    if (i < NUM_ROWS - 1) {
      force += SPRING_CONSTANT * (position[i+1] - position[i]);
    }
    force += restore_factor * (restore_center - position[i]);
    if (velocity[i] > friction_min_velocity) {
      force -= friction_force;
    } else if (velocity[i] < -friction_min_velocity) {
      force += friction_force;
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
          2*M_PI*(duty_phase/auto_impulse_period))*auto_impulse_amplitude;
    } else {
      auto_impulse_period = (random() % 30) + 30;
      auto_impulse_amplitude = (random() % 10)*0.1 + 0.5;
    }
  }
  if (read_button('a') || read_button('b')) {
    auto_impulse = 0;
  }
  if (read_button('x')) {
    button_force = 10;
    restore_factor = 0;
  }
  if (read_button('y')) {
    button_force = 30;
    restore_factor = 1;
    double sum = 0;
    for (int i = 0; i < NUM_ROWS; i++) {
      sum += position[i];
    }
    restore_center = sum / NUM_ROWS;
  }
  
  for (int t = 0; t < TICKS_PER_FRAME; t++) {
    tick(1.0/FPS/TICKS_PER_FRAME);
  }

  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      set_from_palette(pixels, pixel_index(i, j), PALETTE,
                       position[i] + (float) j / NUM_COLUMNS + 0.5);
    }
  }

  for (int s = 0; s < 10; s++) {
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
}
