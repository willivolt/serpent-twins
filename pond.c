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

#define NUM_SEGMENTS 10
#define LENGTH 120
#define CIRC 25
#define PIXELS_PER_SEGMENT (LENGTH*CIRC/NUM_SEGMENTS)
#define FRICTION_MIN_VELOCITY 0.02
#define FRICTION_FORCE 10
#define FPS 10
#define TICKS_PER_FRAME 10
#define DUTY_CYCLE_ON 1.0
#define DUTY_CYCLE_PERIOD 10.0

unsigned char pixels[LENGTH*CIRC*3];
float position[LENGTH][CIRC];
float velocity[LENGTH][CIRC];
#define MASS 1  // kg
#define SPRING_CONSTANT 300  // N/m

void tick(float dt) {
  for (int i = 0; i < LENGTH; i++) {
    for (int j = 0; j < CIRC; j++) {
      float delta = (position[i][(j + 1) % CIRC] - position[i][j]) +
                    (position[i][(j + CIRC - 1) % CIRC] - position[i][j]);
      if (i > 0) {
        delta += (position[i - 1][j] - position[i][j]);
      }
      if (i < LENGTH - 1) {
        delta += (position[i + 1][j] - position[i][j]);
      }
      delta += -position[i][j]*0.02;
      float force = SPRING_CONSTANT * delta;
      if (velocity[i][j] > FRICTION_MIN_VELOCITY) {
        force -= FRICTION_FORCE;
      } else if (velocity[i][j] < -FRICTION_MIN_VELOCITY) {
        force += FRICTION_FORCE;
      }
      velocity[i][j] += force/MASS * dt;
    }
  }
  for (int i = 0; i < LENGTH; i++) {
    for (int j = 0; j < CIRC; j++) {
      position[i][j] += velocity[i][j] * dt;
    }
  }
}

int drop_x, drop_y, last_on = 0;
float drop_impulse = 2000/MASS;
#define ENV_MAP_SIZE 800
unsigned char env_map[2100];
#define ENV_MAP SUNSET_PALETTE

void next_frame(int f) {
  float t = (float) f / FPS;
  if (f == 0) {
    for (int e = 0; e < 400; e++) {
      env_map[e*3] = e/2;
      env_map[e*3+1] = e/4;
      env_map[e*3+2] = 200;
    }
    for (int e = 400; e < 700; e++) {
      env_map[e*3] = 200;
      env_map[e*3+1] = 410 - e*0.4;
      env_map[e*3+2] = 0;
    }
    for (int i = 0; i < LENGTH; i++) {
      for (int j = 0; j < CIRC; j++) {
        position[i][j] = 0;
        velocity[i][j] = 0;
      }
    }
  }

  float duty_phase = t - ((int) (t / DUTY_CYCLE_PERIOD) * DUTY_CYCLE_PERIOD);
  if (duty_phase >= 0 && duty_phase < DUTY_CYCLE_ON) {
    if (last_on == 0) {
      drop_x = rand() % (LENGTH - 1);
      drop_y = rand() % CIRC;
      drop_impulse = -drop_impulse;
    }
    float k = sin(duty_phase/DUTY_CYCLE_ON*M_PI);
    velocity[drop_x][drop_y] += drop_impulse*k;
    velocity[drop_x][(drop_y + 1) % CIRC] += drop_impulse*k;
    velocity[drop_x + 1][drop_y] += drop_impulse*k;
    velocity[drop_x + 1][(drop_y + 1) % CIRC] += drop_impulse*k;
    last_on = 1;
  } else {
    last_on = 0;
  }
  for (int j = 0; j < CIRC; j++) {
    position[0][j] = 0;
    position[LENGTH - 1][j] = 0;
  }
  for (int t = 0; t < TICKS_PER_FRAME; t++) {
    tick(1.0/FPS/TICKS_PER_FRAME);
  }
  for (int i = 0; i < LENGTH; i++) {
    for (int j = 0; j < CIRC; j++) {
      int e = (position[i][j]-position[i+1][j])*300 + ENV_MAP_SIZE/2;
      e = (e < 0) ? 0 : (e > ENV_MAP_SIZE - 1) ? ENV_MAP_SIZE - 1 : e;
      unsigned char* ep = ENV_MAP + e*3;
      set_rgb(pixels, i*CIRC + ((i % 2) ? (CIRC-1-j) : j),
              ep[0]*180/255, ep[1]*180/255, ep[2]*180/255);
    }
  }
  for (int s = 0; s < NUM_SEGMENTS; s++) {
    put_pixels(s, pixels + s*PIXELS_PER_SEGMENT*3, PIXELS_PER_SEGMENT);
  }
}
