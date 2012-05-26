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
#include <string.h>
#include "serpent.h"
#include "spectrum.pal"
#include "sunset.pal"
#define SWIRL_PALETTE SPECTRUM_PALETTE

#define SWIRL_DUTY_CYCLE_ON 120
#define SWIRL_DUTY_CYCLE_OFF 120
#define SWIRL_IMPULSE_START (1*FPS)

#define SWIRL_TICKS_PER_FRAME 10 
#define SWIRL_SPRING_CONST 6000  // kg/s^2
#define SWIRL_MASS 0.1  // kg

unsigned char pixels[NUM_PIXELS*3];
float swirl_position[NUM_ROWS];  // rev
float swirl_velocity[NUM_ROWS];  // rev/s
int swirl_auto_impulse = 1;
float swirl_button_force = 30;
float swirl_restore_factor = 1;
float swirl_restore_center = 0;
float swirl_auto_impulse_period = 40;
float swirl_auto_impulse_amplitude = 0.9;

void swirl_tick(float dt) {
  float friction_force = 0.05 + read_button('y')*0.2;
  float friction_min_velocity = friction_force/SWIRL_MASS * dt;

  for (int i = 0; i < NUM_ROWS; i++) {
    float force;
    if (i == 0) {
      force = accel_right()*0.7 +
          (read_button('b') - read_button('a'))*swirl_button_force;
      if (force < -25 || force > 25) {
        swirl_auto_impulse = 0;
      }
      if (swirl_auto_impulse) {
        continue;
      }
    } else {
      force = SWIRL_SPRING_CONST * (swirl_position[i-1] - swirl_position[i]);
    }
    if (i < NUM_ROWS - 1) {
      force += SWIRL_SPRING_CONST * (swirl_position[i+1] - swirl_position[i]);
    }
    force += swirl_restore_factor * (swirl_restore_center - swirl_position[i]);
    if (swirl_velocity[i] > friction_min_velocity) {
      force -= friction_force;
    } else if (swirl_velocity[i] < -friction_min_velocity) {
      force += friction_force;
    }
    swirl_velocity[i] += force/SWIRL_MASS * dt;
  }
  for (int i = 0; i < NUM_ROWS; i++) {
    swirl_position[i] += swirl_velocity[i] * dt;
  }
}

void next_frame(int x) {
  static int left_outer_eye_start = 182;
  static int right_outer_eye_start = 182 + 22 + 13 + 12 + 6;
  static int outer_eye_length = 22;
  static int inner_eye_length = 13;

  if (x == 0) {
    swirl_auto_impulse = 1;
    for (int i = 0; i < NUM_ROWS; i++) {
      swirl_position[i] = 0;
      swirl_velocity[i] = 0;
    }
  }

  if (swirl_auto_impulse) {
    float duty_phase = (x - SWIRL_IMPULSE_START) %
        (SWIRL_DUTY_CYCLE_ON + SWIRL_DUTY_CYCLE_OFF);
    if (x > SWIRL_IMPULSE_START && duty_phase < SWIRL_DUTY_CYCLE_ON) {
      swirl_position[0] = swirl_auto_impulse_amplitude *
          sin(2*M_PI*(duty_phase/swirl_auto_impulse_period));
    } else {
      swirl_auto_impulse_period = (random() % 30) + 30;
      swirl_auto_impulse_amplitude = ((random() % 10)*0.1 + 0.5)*0.4;
    }
  }
  if (read_button('a') || read_button('b')) {
    swirl_auto_impulse = 0;
  }
  if (read_button('x')) {
    swirl_button_force = 10;
    swirl_restore_factor = 0;
  }
  if (read_button('y')) {
    swirl_button_force = 30;
    swirl_restore_factor = 1;
    double sum = 0;
    for (int i = 0; i < NUM_ROWS; i++) {
      sum += swirl_position[i];
    }
    swirl_restore_center = sum / NUM_ROWS;
  }
  
  for (int t = 0; t < SWIRL_TICKS_PER_FRAME; t++) {
    swirl_tick(1.0/FPS/SWIRL_TICKS_PER_FRAME);
  }

  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      set_from_palette(pixels, pixel_index(i, j), SWIRL_PALETTE,
                       swirl_position[i] + (float) j / NUM_COLUMNS + 0.5);
    }
  }

  for (int s = 0; s < 10; s++) {
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }

  if (strcmp(get_button_sequence(), "xxaxa") == 0) {
    left_outer_eye_start -= 1;
    clear_button_sequence();
  }
  if (strcmp(get_button_sequence(), "xxaxb") == 0) {
    left_outer_eye_start += 1;
    clear_button_sequence();
  }
  if (strcmp(get_button_sequence(), "xxbxa") == 0) {
    right_outer_eye_start -= 1;
    clear_button_sequence();
  }
  if (strcmp(get_button_sequence(), "xxbxb") == 0) {
    right_outer_eye_start += 1;
    clear_button_sequence();
  }

  // overwrite the eyes
  pixel* p = (pixel*) pixels;
  for (int i = 0; i < outer_eye_length; i++) {
    p[left_outer_eye_start + i] = p[0];
    p[right_outer_eye_start + i] = p[0];
  }
  for (int i = 0; i < inner_eye_length; i++) {
    p[left_outer_eye_start + outer_eye_length + i] = p[NUM_PIXELS - 1];
    p[right_outer_eye_start + outer_eye_length + i] = p[NUM_PIXELS - 1];
  }
  put_head_pixels(pixels, HEAD_PIXELS);
}
