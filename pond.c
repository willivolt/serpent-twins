// "pond", by Ka-Ping Yee

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
#include "sunset.pal"
#include "spectrum.pal"

#define POND_FRICTION_MIN_VELOCITY 0.02
#define POND_FRICTION_FORCE 10
#define POND_TICKS_PER_FRAME 10
#define POND_DUTY_CYCLE_ON 1.0
#define POND_DUTY_CYCLE_PERIOD 10.0
#define POND_TIME_SPEEDUP 2

unsigned char pixels[NUM_ROWS*NUM_COLUMNS*3];
float pond_position[NUM_ROWS][NUM_COLUMNS];
float pond_velocity[NUM_ROWS][NUM_COLUMNS];
#define POND_MASS 1  // kg
#define POND_SPRING_CONSTANT 300  // N/m

void pond_tick(float dt) {
  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      float delta =
          (pond_position[i][(j + 1) % NUM_COLUMNS] - pond_position[i][j]) +
          (pond_position[i][(j + NUM_COLUMNS - 1) % NUM_COLUMNS] -
              pond_position[i][j]);
      if (i > 0) {
        delta += (pond_position[i - 1][j] - pond_position[i][j]);
      }
      if (i < NUM_ROWS - 1) {
        delta += (pond_position[i + 1][j] - pond_position[i][j]);
      }
      delta += -pond_position[i][j]*0.02;
      float force = POND_SPRING_CONSTANT * delta;
      if (pond_velocity[i][j] > POND_FRICTION_MIN_VELOCITY) {
        force -= POND_FRICTION_FORCE;
      } else if (pond_velocity[i][j] < -POND_FRICTION_MIN_VELOCITY) {
        force += POND_FRICTION_FORCE;
      }
      pond_velocity[i][j] += force/POND_MASS * dt;
    }
  }
  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      pond_position[i][j] += pond_velocity[i][j] * dt;
    }
  }
}

int pond_drop_x, pond_drop_y, pond_last_on = 0;
float pond_drop_impulse = 2000/POND_MASS;
#define POND_ENV_MAP_SIZE 800
unsigned char pond_env_map[2400];
#define POND_ENV_MAP pond_env_map
typedef unsigned short word;

void next_frame(int f) {
  float t = POND_TIME_SPEEDUP * (float) f / FPS;
  if (f == 0) {
    for (int e = 0; e < 400; e++) {
      pond_env_map[e*3] = e/2;
      pond_env_map[e*3+1] = e/4;
      pond_env_map[e*3+2] = 200;
    }
    for (int e = 400; e < 700; e++) {
      pond_env_map[e*3] = 200;
      pond_env_map[e*3+1] = 410 - e*0.4;
      pond_env_map[e*3+2] = 0;
    }
    byte* sunset = SUNSET_PALETTE;
    byte* p = pond_env_map;
    for (int e = 0; e < POND_ENV_MAP_SIZE; e++) {
      byte r = *(sunset++);
      byte g = *(sunset++);
      byte b = *(sunset++);
      short rr = ((short) (r - 128))*1.2 + 128;
      short gg = ((short) (g - 128))*1.2 + 128;
      short bb = ((short) (b - 128))*1.2 + 128;
      *(p++) = rr < 0 ? 0 : rr > 255 ? 255 : rr;
      *(p++) = gg < 0 ? 0 : gg > 255 ? 255 : gg;
      *(p++) = bb < 0 ? 0 : bb > 255 ? 255 : bb;
      /*
      byte value = e*128/POND_ENV_MAP_SIZE;
      *(p++) = ((word) r * value)>>8;
      *(p++) = ((word) g * value)>>8;
      *(p++) = ((word) b * value)>>8;
      */
      /*
      *(p++) = r; //((word) r * (255 - r)) >> 8;
      *(p++) = ((word) g * (255 - r)) >> 8;
      *(p++) = ((word) b * (255 - r)) >> 8;
      */
    }
    for (int i = 0; i < NUM_ROWS; i++) {
      for (int j = 0; j < NUM_COLUMNS; j++) {
        pond_position[i][j] = 0;
        pond_velocity[i][j] = 0;
      }
    }
  }

  float duty_phase =
      t - ((int) (t / POND_DUTY_CYCLE_PERIOD) * POND_DUTY_CYCLE_PERIOD);
  if (duty_phase >= 0 && duty_phase < POND_DUTY_CYCLE_ON) {
    if (pond_last_on == 0) {
      pond_drop_x = rand() % (NUM_ROWS - 1);
      pond_drop_y = rand() % NUM_COLUMNS;
      pond_drop_impulse = -pond_drop_impulse;
    }
    float k = sin(duty_phase/POND_DUTY_CYCLE_ON*M_PI);
    pond_velocity[pond_drop_x][pond_drop_y] += pond_drop_impulse*k;
    pond_velocity[pond_drop_x][(pond_drop_y + 1) % NUM_COLUMNS] +=
        pond_drop_impulse*k;
    pond_velocity[pond_drop_x + 1][pond_drop_y] += pond_drop_impulse*k;
    pond_velocity[pond_drop_x + 1][(pond_drop_y + 1) % NUM_COLUMNS] +=
        pond_drop_impulse*k;
    pond_last_on = 1;
  } else {
    pond_last_on = 0;
  }
  for (int j = 0; j < NUM_COLUMNS; j++) {
    pond_position[0][j] = 0;
    pond_position[NUM_ROWS - 1][j] = 0;
  }
  for (int t = 0; t < POND_TICKS_PER_FRAME; t++) {
    pond_tick(POND_TIME_SPEEDUP * 1.0/FPS/POND_TICKS_PER_FRAME);
  }
  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      int e = (pond_position[i][j]-pond_position[i+1][j])*300 +
          POND_ENV_MAP_SIZE/2;
      e = (e < 0) ? 0 : (e > POND_ENV_MAP_SIZE - 1) ?
          POND_ENV_MAP_SIZE - 1 : e;
      unsigned char* ep = POND_ENV_MAP + e*3;
      set_rgb(pixels, i*NUM_COLUMNS + ((i % 2) ? (NUM_COLUMNS-1-j) : j),
              ep[0]*180/255, ep[1]*180/255, ep[2]*180/255);
    }
  }
  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
}
