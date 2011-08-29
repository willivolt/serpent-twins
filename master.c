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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serpent.h"

#define SEC FPS  // use this for animation time parameters
//#define SEC 6  // use this to speed up by a factor of 10 for testing


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

// sets the colour of a pixel from a palette
#define paint_from_palette(pixels, pixel_index, palette, f, alpha) { \
  byte* __s = (palette) + palette_index(palette, f)*3; \
  paint_rgb(pixels, pixel_index, __s[0], __s[1], __s[2], alpha); \
}


// Common initialization ===================================================

byte ease[257];
#define EASE(frame, period) ease[((frame) << 8) / (period)]

float sin_table[256];
#define SIN(n) sin_table[((int) (n)) & 0xff]
#define COS(n) sin_table[((int) (n) + 0x40) & 0xff]

void init_tables() {
  short i;
  float min = tanh(-2.56), span = tanh(2.56) - tanh(-2.56);
  for (i = 0; i <= 256; i++) {
    ease[i] = ((tanh(-2.56 + i*0.02)-min)/span)*255 + 0.5;
  }
  for (i = 0; i < 256; i++) {
    sin_table[i] = sin(i/256.0*2*M_PI);
  }
}


// Pattern state and transitions ===========================================

struct pattern;
typedef byte next_frame_func(struct pattern* p, pixel* pixels);
struct pattern {
  char* name;
  next_frame_func* next_frame;
  long frame;
};
typedef struct pattern pattern;

short transition_alpha(
    long frame, long in_period, long duration, long out_period) {
  if (frame > in_period + duration + out_period) {
    return -1;
  }
  if (frame > in_period + duration) {
    return 256 - EASE(frame - in_period - duration, out_period);
  }
  if (frame < in_period) {
    return EASE(frame, in_period);
  }
  return 256;
}

#define get_alpha_or_terminate(frame, in_period, duration, out_period) \
  (__alpha = transition_alpha(frame, in_period, duration, out_period)); \
  if (__alpha < 0) return 0;


// Null pattern ============================================================

byte null_next_frame(pattern* p, pixel* pixels) {
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

byte base_next_frame(pattern* p, pixel* pixels) {
  static pixel source_pixels[NUM_PIXELS];
  static pixel target_pixels[NUM_PIXELS];
  static long in_period = 0;
  static long duration = 0;
  short alpha;

  if (p->frame < in_period + duration) {
    alpha = transition_alpha(p->frame, in_period, duration, 1);
  } else {
    memcpy(source_pixels, target_pixels, NUM_PIXELS*3);
    base_select_target(target_pixels);
    p->frame = 0;
    in_period = 1*SEC + random() % (20*SEC);
    duration = random() % (40*SEC);
    alpha = 0;
  }

  for (short i = 0; i < NUM_PIXELS; i++) {
    pixels[i] = source_pixels[i];
    paint_pixel(pixels, i, target_pixels[i], alpha);
  }

  return 1;
}


// "swirl", by Ka-Ping Yee =================================================

#include "spectrum.pal"
#include "sunset.pal"
#define SWIRL_PALETTE SPECTRUM_PALETTE

#define SWIRL_DUTY_CYCLE_ON 120
#define SWIRL_DUTY_CYCLE_OFF 120
#define SWIRL_IMPULSE_START (60*SEC)

#define SWIRL_TICKS_PER_FRAME 10 
#define SWIRL_SPRING_CONST 400  // kg/s^2
#define SWIRL_MASS 0.1  // kg

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

byte swirl_next_frame(pattern* p, pixel* pixels) {
  int x = p->frame;
  short alpha = get_alpha_or_terminate(x, 3*SEC, 2*60*SEC, 10*SEC);

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
      swirl_auto_impulse_amplitude = (random() % 10)*0.1 + 0.5;
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
      paint_from_palette(pixels, pixel_index(i, j), SWIRL_PALETTE,
                         swirl_position[i] + (float) j / NUM_COLUMNS + 0.5,
                         alpha);
    }
  }

  return 1;
}


// "rabbit-sine", by David Rabbit Wallace ==================================

#define RABBIT_MAX_BRIGHT 255

byte rabbit_sine_next_frame(pattern* p, pixel* pixels) {
    int frame = p->frame;
    short alpha = get_alpha_or_terminate(frame, 10*SEC, 60*SEC, 10*SEC);

    int r,g,b,temp;

    // coordinates
    int x,y;         // x is around, y is along
    float alt;       // altitude of each point, from -1 to 1
    int seg;         // which trailer?

    // scalar field values
    float sin_small;
    float sin_large;
    float bright;

    // movement and look
    float wavelength_small = 12.0;
    float wavelength_large = 64.0;
    float time_offset_small = -frame*0.5;  // larger number == faster movement
    float time_offset_large = frame*0.9;
    float black_stripe_width = 0.8; // width of black stripe between colors
    

    for (int i = 0; i < 300*NUM_SEGS; i++) {
        //-------------------------------------------
        // RADIAL COORDINATES
        x = (i % NUM_COLUMNS);  // theta.  0 to 24
        y = (i / NUM_COLUMNS);  // along cylinder.  0 to NUM_SEGS*SEG_ROWS (120)
        seg = y / SEG_ROWS;
        // reverse every other row
        if (y % 2 == 1) {
            x = (NUM_COLUMNS-1)-x;
        }
        
        // rotate 90 degrees to one side
        //x = (x + NUM_COLUMNS/4) % NUM_COLUMNS;

        // twist like a candy cane
        //x = (x + y*0.6);
        //x = x % NUM_COLUMNS;

        // spin like a rolling log
        //x = (x + frame) % NUM_COLUMNS;

        //-------------------------------------------
        // ALTITUDE
//        alt = -cos(   x/(NUM_COLUMNS-1.0) * 2*M_PI   );
        alt = -COS(   x/(NUM_COLUMNS-1.0) * 256   );

        //-------------------------------------------
        // BRIGHTNESS

        // combine two sine waves along the length of the snake with the altitude at each point
        sin_small = SIN(   (time_offset_small + y) / wavelength_small * 256   );
        sin_large = SIN(   (time_offset_large + y) / wavelength_large * 256   );
//        sin_small = sin(   (time_offset_small + y) / wavelength_small * 2*M_PI   );
//        sin_large = sin(   (time_offset_large + y) / wavelength_large * 2*M_PI   );
        bright = (sin_small*0.5 + sin_large*0.7)*0.7 + alt*0.5;

        // increase contrast and invert
        bright *= -2.0;

        //-------------------------------------------
        // CONVERT BRIGHTNESS TO COLOR

        // put a stripe down the middle of the black part
        if (0 && (-black_stripe_width < bright) && (bright < black_stripe_width)) {
            if (bright < 0) { bright = -bright; }
            bright = bright / black_stripe_width;
            bright = 1-bright;
            bright = (bright - 0.8) / (1-0.8);
            r = bright * 1.0 * RABBIT_MAX_BRIGHT;
            g = bright * 0.0 * RABBIT_MAX_BRIGHT;
            b = bright * 2.0 * RABBIT_MAX_BRIGHT;
        } else {
            if (bright > 0) {
                // blue part for positive brightness
                bright = bright - black_stripe_width;
                r = bright * 0.1 * RABBIT_MAX_BRIGHT;
                g = bright * 0.5 * RABBIT_MAX_BRIGHT;
                b = bright * 2.0 * RABBIT_MAX_BRIGHT;
            } else {
                // orange part for negative brightness
                bright = -bright - black_stripe_width;
                r = bright * 2.0 * RABBIT_MAX_BRIGHT;
                g = bright * 0.5 * RABBIT_MAX_BRIGHT;
                b = bright * 0.1 * RABBIT_MAX_BRIGHT;
            }
        }

        
        //-------------------------------------------
        // SHIFT COLORS ON EACH SEGMENT
        //if (seg%6 == 1) {
        //    temp = r;
        //    r = g;
        //    g = b;
        //    b = temp;
        //}
        //if (seg%6 == 2) {
        //    temp = r;
        //    r = b;
        //    b = g;
        //    g = temp;
        //}
        //if (seg%6 == 3) {
        //    temp = r;
        //    r = g;
        //    g = temp;
        //}
        //if (seg%6 == 4) {
        //    temp = r;
        //    r = b;
        //    b = temp;
        //}
        //if (seg%6 == 5) {
        //    temp = b;
        //    b = g;
        //    g = temp;
        //}


        //-------------------------------------------
        // STORE RESULT
        if (r < 0) { r = 0; }
        if (r > RABBIT_MAX_BRIGHT) { r = RABBIT_MAX_BRIGHT; }
        if (g < 0) { g = 0; }
        if (g > RABBIT_MAX_BRIGHT) { g = RABBIT_MAX_BRIGHT; }
        if (b < 0) { b = 0; }
        if (b > RABBIT_MAX_BRIGHT) { b = RABBIT_MAX_BRIGHT; }
        paint_rgb(pixels, i, r, g, b, alpha);
    }

    return 1;
}


// "electric", by Christopher De Vries =====================================

#define ELECTRIC_LOCI 200

void electric_draw_locus(int x, int orientation, byte* pixels) {
  int i;

  for(i=(x*25+orientation)*3;i<(x*25+25)*3;i+=9) {
    pixels[i]=0x80;
    pixels[i+1]=0x00;
    pixels[i+2]=0xff;
  }
}

byte electric_next_frame(pattern* p, pixel* pixels) {
  static int inv_velocity[ELECTRIC_LOCI];
  static int inv_omega[ELECTRIC_LOCI];
  static int t_not[ELECTRIC_LOCI];
  static int nnext=0;
  static int nmax=0;
  static byte temp_pixels[NUM_PIXELS*3];
  int i;
  int posx;
  int orientation;

  short alpha = get_alpha_or_terminate(p->frame, 1*SEC, 2*60*SEC, 10*SEC);
  int frame = p->frame - 9*SEC;

  for(i=0;i<9000;i++) {
    temp_pixels[i]=0;
  }

  if (frame>=0) {
    if(frame%10==0) {
      inv_velocity[nnext]=1000/(frame%1000+1);
      inv_omega[nnext]=333/(frame%333+1);
      t_not[nnext]=frame;
 
      nnext++;
      if(nnext==ELECTRIC_LOCI) nnext=0;
 
      if(nmax<ELECTRIC_LOCI) {
        nmax++;
      }
    }
 
    for(i=0;i<nmax;i++) {
      posx=(frame-t_not[i])/inv_velocity[i]%120;
      orientation=(frame-t_not[i])/inv_omega[i]%3;
      electric_draw_locus(posx,orientation,temp_pixels);
    }
  }

  byte* t = temp_pixels;
  for(i=0;i<NUM_PIXELS;i++) {
    byte r = *(t++);
    byte g = *(t++);
    byte b = *(t++);
    paint_rgb(pixels, i, r, g, b, alpha); 
  }

  return 1;
}


// Flash pattern ===========================================================

byte flash_next_frame(pattern* p, pixel* pixels) {
  add_to_pixel(pixels, random() % NUM_PIXELS, 255, 255, 255, 256);

  return p->frame < 60*SEC;
}


// Pattern table ===========================================================

pattern BASE_PATTERN = {"base", base_next_frame, 0};

#define NUM_PATTERNS 3
pattern PATTERNS[] = {
//  {plasma_next_frame, 0},
  {"swirl", swirl_next_frame, 0},
  {"rabbit-sine", rabbit_sine_next_frame, 0},
  {"electric", electric_next_frame, 0},
};


// Master routine ==========================================================

static pattern* curp = NULL;

void activate_pattern(pattern* p) {
  printf("\nactivating %s\n", p->name);
  curp = p;
  curp->frame = 0;
}

void next_frame(int frame) {
  static long time_to_next_pattern = 5*SEC;
  static int left_outer_eye_start = 182;
  static int right_outer_eye_start = 182 + 22 + 13 + 12 + 6;
  static int outer_eye_length = 22;
  static int inner_eye_length = 13;
  static int next_pattern = 0;

  if (frame == 0) {
    init_tables();
  }

  base_next_frame(&BASE_PATTERN, pixels);
  BASE_PATTERN.frame++;

  if (curp) {
    if ((curp->next_frame)(curp, pixels)) {
      curp->frame++;
    } else {
      printf("\nfinished %s\n", curp->name);
      curp = NULL;
    }
  } else {
    if (time_to_next_pattern) {
      time_to_next_pattern--;
    } else {
      activate_pattern(PATTERNS + next_pattern);
      time_to_next_pattern = 10*SEC;
      next_pattern = random() % NUM_PATTERNS;
    }
  }

  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, (byte*) (pixels + s*SEG_PIXELS), SEG_PIXELS);
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
  put_head_pixels((byte*) pixels, HEAD_PIXELS);
}
