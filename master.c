// master pattern controller, by Ka-Ping Yee

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

//#define SEC FPS  // use this for animation time parameters
#define SEC 6  // use this to speed up by a factor of 10 for testing


// Pixel manipulation ======================================================

pixel pixels[NUM_PIXELS];

short __i;
long __alpha;
long __value;

#define paint_rgb(pixels, i, red, green, blue, alpha) { \
  __i = i; \
  __alpha = alpha; \
  __value = (((long) red)*(__alpha) + pixels[__i].r*(256-__alpha)) >> 8; \
  pixels[__i].r = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
  __value = (((long) green)*(__alpha) + pixels[__i].g*(256-__alpha)) >> 8; \
  pixels[__i].g = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
  __value = (((long) blue)*(__alpha) + pixels[__i].b*(256-__alpha)) >> 8; \
  pixels[__i].b = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
}

#define paint_pixel(pixels, i, pix, alpha) \
    paint_rgb(pixels, i, pix.r, pix.g, pix.b, alpha)

#define add_to_pixel(pixels, i, red, green, blue, alpha) { \
  __i = i; \
  __alpha = alpha; \
  __value = ((((long) red)*(__alpha)) >> 8) + pixels[__i].r; \
  pixels[__i].r = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
  __value = ((((long) green)*(__alpha)) >> 8) + pixels[__i].g; \
  pixels[__i].g = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
  __value = ((((long) blue)*(__alpha)) >> 8) + pixels[__i].b; \
  pixels[__i].b = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
}


// Common initialization ===================================================

byte ease[257];
#define EASE(frame, period) ease[((frame) << 8) / (period)]

float sin_table[512];
#define SIN(n) sin_table[((short) (n*2)) & 0x1ff]

void init_tables() {
  short i;
  float min = tanh(-2.56), span = tanh(2.56) - tanh(-2.56);
  for (i = 0; i <= 256; i++) {
    ease[i] = ((tanh(-2.56 + i*0.02)-min)/span)*255 + 0.5;
  }
  for (i = 0; i < 512; i++) {
    sin_table[i] = sin(i/512.0*2*M_PI);
  }
}


// Pattern state and transitions ===========================================

struct pattern;
typedef byte next_frame_func(struct pattern* p, pixel* pixels);
struct pattern {
  next_frame_func* next_frame;
  long frame;
};

short transition_alpha(long frame, long stop_frame, long transition_period) {
  if (frame > stop_frame + transition_period) {
    return -1;
  }
  if (frame > stop_frame) {
    return 256 - EASE(frame - stop_frame, transition_period);
  }
  if (frame < transition_period) {
    return EASE(frame, transition_period);
  }
  return 256;
}

#define get_alpha_or_terminate(frame, duration, transition_duration) \
  (__alpha = transition_alpha(frame, duration, transition_duration)); \
  if (__alpha < 0) return 0;


// Null pattern ============================================================

byte null_next_frame(struct pattern* p, pixel* pixels) {
  return 0;
}


// Base pattern ============================================================

void base_select_target(pixel* target_pixels) {
  pixel h = {random() % 250 + 5, random() % 250 + 5, random() % 250 + 5};
  pixel t = {random() % 250 + 5, random() % 250 + 5, random() % 250 + 5};
  for (short r = 0; r < NUM_ROWS; r++) {
    short alpha = (long) r*256/NUM_ROWS;
    for (short c = 0; c < NUM_COLUMNS; c++) {
      short i = pixel_index(r, c);
      set_pixel(target_pixels, i, h);
      paint_pixel(target_pixels, i, t, alpha);
    }
  }
}

byte base_next_frame(struct pattern* p, pixel* pixels) {
  static pixel source_pixels[NUM_PIXELS];
  static pixel target_pixels[NUM_PIXELS];
  static long stop_frame = 0;
  static long transition_period = 0;
  short alpha;

  if (p->frame < stop_frame) {
    alpha = transition_alpha(p->frame, stop_frame, transition_period);
  } else {
    memcpy(source_pixels, target_pixels, NUM_PIXELS*3);
    base_select_target(target_pixels);
    p->frame = 0;
    transition_period = 1*SEC + random() % (20*SEC);
    stop_frame = transition_period + random() % (40*SEC);
    alpha = 0;
  }

  for (short i = 0; i < NUM_PIXELS; i++) {
    pixels[i] = source_pixels[i];
    paint_pixel(pixels, i, target_pixels[i], alpha);
  }

  return 1;
}


// Flash pattern ===========================================================

byte flash_next_frame(struct pattern* p, pixel* pixels) {
  add_to_pixel(pixels, random() % NUM_PIXELS, 255, 255, 255, 256);

  return p->frame < 60*SEC;
}


// Pattern table ===========================================================

struct pattern NULL_PATTERN = {null_next_frame, 0};

struct pattern BASE_PATTERN = {base_next_frame, 0};

#define NUM_PATTERNS 1
struct pattern PATTERNS[] = {
//  {plasma_next_frame, 0},
  {flash_next_frame, 0}
};


// Master routine ==========================================================

static struct pattern* curp = &NULL_PATTERN;

void activate_pattern(struct pattern* p) {
  curp = p;
  curp->frame = 0;
}

void next_frame(int frame) {
  static long time_to_next_pattern = 60*SEC;

  if (frame == 0) {
    init_tables();
  }

  base_next_frame(&BASE_PATTERN, pixels);
  BASE_PATTERN.frame++;

  if ((curp->next_frame)(curp, pixels)) {
    curp->frame++;
  } else {
    curp = &NULL_PATTERN;
    if (time_to_next_pattern) {
      time_to_next_pattern--;
    } else {
      activate_pattern(PATTERNS + random() % NUM_PATTERNS);
      time_to_next_pattern = 300*SEC;
    }
  }

  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, (byte*) (pixels + s*SEG_PIXELS), SEG_PIXELS);
  }
}
