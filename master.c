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

#define __USE_SVID

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "serpent.h"
#include "spectrum.pal"
#include "sunset.pal" 
#include "midi.h"


#define SEC FPS  // use this for animation time parameters
//#define SEC 6  // use this to speed up by a factor of 10 for testing

#define in_interval(val, min, count) ((val) >= (min) && (val) < (min) + (count))
#define TAIL_FIN_START 100
#define TAIL_FIN_COUNT 12
#define TAIL_LANTERN_START 112
#define TAIL_LANTERN_COUNT 22

#define CRYSTAL_START 228
#define CRYSTAL_COUNT 6

float last_frame = 0;

// Pixel manipulation ======================================================

pixel head[HEAD_PIXELS];
pixel pixels[NUM_PIXELS];
pixel spine[NUM_ROWS];

short __i;
long __alpha;
long __value;

#define paint_rgb(pixels, i, red, green, blue, alpha) { \
  __i = i; \
  __alpha = alpha; \
  __value = (((long) red)*(__alpha) + (pixels)[__i].r*(256-__alpha)) >> 8; \
  (pixels)[__i].r = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
  __value = (((long) green)*(__alpha) + (pixels)[__i].g*(256-__alpha)) >> 8; \
  (pixels)[__i].g = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
  __value = (((long) blue)*(__alpha) + (pixels)[__i].b*(256-__alpha)) >> 8; \
  (pixels)[__i].b = __value < 0 ? 0 : __value > 255 ? 255 : __value; \
}

#define paint_pixel(pixels, i, pix, alpha) \
    paint_rgb(pixels, i, (pix).r, (pix).g, (pix).b, alpha)

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

#define clamp(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

/* hue = 0..254, sat = 0..255, val = 0..255 */
void hsv_to_rgb(byte hue, byte sat, byte val, pixel* pix) {
  byte r = 0, g = 0, b = 0, max = 0;
  byte chr = (val*sat + 255)>>8;
  byte min = val - chr;
  byte phase = hue % 85;
  byte x;

  phase = (phase >= 43) ? 85 - phase : phase;
  x = ((unsigned int) chr * phase*6 + 127) / 255;
  switch ((hue*2)/85) {
    case 0:
    case 6:
      pix->r = val;
      pix->g = x + min;
      pix->b = min;
      break;
    case 1:
      pix->r = x + min;
      pix->g = val;
      pix->b = min;
      break;
    case 2:
      pix->r = min;
      pix->g = val;
      pix->b = x + min;
      break;
    case 3:
      pix->r = min;
      pix->g = x + min;
      pix->b = val;
      break;
    case 4:
      pix->r = x + min;
      pix->g = min;
      pix->b = val;
      break;
    case 5:
      pix->r = val;
      pix->g = min;
      pix->b = x + min;
      break;
  }
}

#define frandom(max) ((random() % 1000000) * 0.000001 * (max))
#define irandom(max) (random() % (max))

void setup_tint(float hue, float amount, float dim, pixel* p, byte* alpha) {
  byte sat;

  if (amount <= 1) {
    sat = 255;
    *alpha = amount*255.99;
  } else {
    sat = (2 - amount)*255.99;
    *alpha = 255.99;
  }
  hsv_to_rgb(hue*254.99, sat, dim*255.99, p);
}


// Head pixels =============================================================

int left_outer_eye_start;
int right_outer_eye_start;
int outer_eye_length;
int left_inner_eye_start;
int right_inner_eye_start;
int inner_eye_length;
int left_medallion_start;
int right_medallion_start;
int medallion_length;

void init_head_pixel_locations() {
  left_outer_eye_start = 182;
  right_outer_eye_start =
      getenv("BLACK_SERPENT") ? 182 + 22 + 13 + 12 : 182 + 22 + 13 + 12 + 6;
  outer_eye_length = 22;
  left_inner_eye_start = left_outer_eye_start + outer_eye_length;
  right_inner_eye_start = right_outer_eye_start + outer_eye_length;
  inner_eye_length = 13;
  left_medallion_start = 0;
  right_medallion_start =
      right_outer_eye_start + outer_eye_length + inner_eye_length;
  medallion_length = 28;
}

void copy_body_to_head(pixel* pixels, pixel* head) {
  memcpy(head, pixels, sizeof(pixel)*HEAD_PIXELS);

  // Give special colour separation to the eyes.
  pixel* p = (pixel*) pixels;
  for (int i = 0; i < outer_eye_length; i++) {
    head[left_outer_eye_start + i] = pixels[NUM_PIXELS - 1 - i];
    head[right_outer_eye_start + i] = pixels[NUM_PIXELS - 1 - i];
  }
  for (int i = 0; i < inner_eye_length; i++) {
    head[left_outer_eye_start + outer_eye_length + i] = pixels[i];
    head[right_outer_eye_start + outer_eye_length + i] = pixels[i];
  }
}


// Common initialization ===================================================

byte ease[257];
#define EASE(frame, period) ease[(int) (((frame) * 256) / (period))]

float sin_table[256];
#define SIN(n) sin_table[((int) (n)) & 0xff]
#define COS(n) sin_table[((int) (n) + 0x40) & 0xff]
#define SIN256(n) sin_table[((int) ((n)*256)) & 0xff]

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
typedef byte next_frame_func(struct pattern* p, pixel* pixels, pixel* head);
struct pattern {
  char* name;
  next_frame_func* next_frame;
  byte time_warp_capable;
  float frame;
};
typedef struct pattern pattern;
int next_pattern_override_start = -1;
int auto_advance = 0;

short transition_alpha(
    float frame, long in_period, long duration, long out_period) {
  if (!auto_advance) {
    duration = 60*60*SEC;
  }
  // Set next_pattern_override_start to immediately begin a 3-second fade-out.
  if (next_pattern_override_start > 0) {
    frame = frame - next_pattern_override_start + in_period + duration;
    out_period = 3*SEC;
  }
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

byte null_next_frame(pattern* p, pixel* pixels, pixel* head) {
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

byte base_next_frame(pattern* p, pixel* pixels, pixel* head) {
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

#define SWIRL_PALETTE SPECTRUM_PALETTE

#define SWIRL_DUTY_CYCLE_ON 120
#define SWIRL_DUTY_CYCLE_OFF 120
#define SWIRL_IMPULSE_START (15*SEC)

#define SWIRL_TICKS_PER_FRAME 10 
#define SWIRL_SPRING_CONST 400  // kg/s^2
#define SWIRL_MASS 0.1  // kg

float swirl_position[NUM_ROWS];  // rev
float swirl_velocity[NUM_ROWS];  // rev/s
int swirl_auto_impulse = 1;
float swirl_button_force = 30;
float swirl_restore_factor = 0;
float swirl_restore_center = 0;
float swirl_auto_impulse_period = 40;
float swirl_auto_impulse_amplitude = 0.5;

void swirl_tick(float dt) {
  float friction_force = 0.1; // 05 + read_button('y')*0.2;
  float friction_min_velocity = friction_force/SWIRL_MASS * dt;
  float spring = midi_get_control(7) / 100.0 + 0.5;
  spring = spring * spring;
  spring = SWIRL_SPRING_CONST * spring * spring;

  for (int i = 0; i < NUM_ROWS; i++) {
    float force;
    if (i == 0) {
      force = accel_right()*0.7 +
          (read_button('b') - read_button('a'))*swirl_button_force;
      if (force < -25 || force > 25 || midi_get_control(8) != 64) {
        swirl_auto_impulse = 0;
      }
      if (swirl_auto_impulse) {
        continue;
      }
      swirl_position[i] = (midi_get_control(8) - 64)/80.0;
    } else {
      force = spring * (swirl_position[i-1] - swirl_position[i]);
    }
    if (i < NUM_ROWS - 1) {
      force += spring * (swirl_position[i+1] - swirl_position[i]);
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

byte swirl_next_frame(pattern* p, pixel* pixels, pixel* head) {
  int x = p->frame;
  short alpha = get_alpha_or_terminate(x, 3*SEC, 2*60*SEC, 3*SEC);

  if (x == 0) {
    midi_set_control_with_pickup(7, 40);
    midi_set_control_with_pickup(8, 64);
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

  if (p->frame != last_frame) {
    int count = SWIRL_TICKS_PER_FRAME*(p->frame - last_frame);
    if (count < 1) { count = 1; }
    for (int t = 0; t < count; t++) {
      swirl_tick(1.0/FPS/SWIRL_TICKS_PER_FRAME);
    }
    last_frame = p->frame;
  }

  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      paint_from_palette(pixels, pixel_index(i, j), SWIRL_PALETTE,
                         swirl_position[i] + (float) j / NUM_COLUMNS + 0.5,
                         alpha);
    }
  }

  copy_body_to_head(pixels, head);
  return 1;
}


// "rabbit-sine", by David Rabbit Wallace ==================================

#define RABBIT_MAX_BRIGHT 255

byte rabbit_sine_next_frame(pattern* p, pixel* pixels, pixel* head) {
    float frame = p->frame;
    short alpha = get_alpha_or_terminate(frame, 3*SEC, 2*60*SEC, 3*SEC);

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

    copy_body_to_head(pixels, head);
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

byte electric_next_frame(pattern* p, pixel* pixels, pixel* head) {
  static int inv_velocity[ELECTRIC_LOCI];
  static int inv_omega[ELECTRIC_LOCI];
  static int t_not[ELECTRIC_LOCI];
  static int nnext=0;
  static int nmax=0;
  static byte temp_pixels[NUM_PIXELS*3];
  int i;
  int posx;
  int orientation;

  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 2*60*SEC, 3*SEC);
  int frame = p->frame - 4*SEC;

  for(i=0;i<9000;i++) {
    temp_pixels[i]=0;
  }

  if(frame>=0) {
    if(frame%10==0 && frame != last_frame) {
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

    last_frame = frame;
  }

  byte* t = temp_pixels;
  for(i=0;i<NUM_PIXELS;i++) {
    byte r = *(t++);
    byte g = *(t++);
    byte b = *(t++);
    paint_rgb(pixels, i, r, g, b, alpha); 
  }

  copy_body_to_head(pixels, head);
  return 1;
}


// "squares", by Brian Schantz =============================================

#define SQUARES_NUM_SPRITES 90
#define SQUARES_MAX_W 16      // max width (around circ.) of a squares_sprite
#define SQUARES_MAX_H 24      // max height (length of serpent) of a squares_sprite
#define SQUARES_MAX_Z 100     // max # of Z-levels (could improve this by making sure no two squares_sprites are on the same Z-level)
#define SQUARES_MAX_DX 2.0      // max X-axis movement in pix/frame
#define SQUARES_MAX_DY 6.0      // max Y-axis movement in pix/frame
#define SQUARES_MAX_BG_BLINK 160  // max # of frames between flashes

typedef struct {
  int w,h,z;
  float x,y;
  int r,g,b;
  float dx, dy;
} squares_sprite;

typedef struct {
  int r,g,b;
} squares_color;

void squares_init_sprites(void);
squares_sprite* squares_top_sprite(int x, int y);
void squares_move_sprites(float df);

squares_sprite squares_sprites[SQUARES_NUM_SPRITES];
squares_color squares_bg;
int squares_blinktimer;

byte squares_next_frame(pattern* p, pixel* pixels, pixel* head) {
  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 2*60*SEC, 3*SEC);
  int frame = p->frame;

  if (frame == 0 || midi_get_control(14)) {
    squares_init_sprites();
  }

  switch(squares_blinktimer) {
    case 5:
      squares_bg.r = squares_bg.g = squares_bg.b = 128;
      break;
    case 4:
      squares_bg.r = squares_bg.g = squares_bg.b = 255;
      break;
    case 3:
      squares_bg.r = squares_bg.g = squares_bg.b = 192;
      break;
    case 2:
      squares_bg.r = squares_bg.g = squares_bg.b = 128;
      break;
    case 1:
      squares_bg.r = squares_bg.g = squares_bg.b = 64;
      break;
    case 0:
      squares_blinktimer = rand()%SQUARES_MAX_BG_BLINK;
      // fall through intentionally
    default:
      squares_bg.r = squares_bg.g = squares_bg.b = 0;
      break;
  }
  if (p->frame != last_frame) {
    squares_blinktimer--;
  }
      
    for (int i = 0; i < NUM_PIXELS; i++) {
      int x = (i % NUM_COLUMNS);  // theta.  0 to 24
        int y = (i / NUM_COLUMNS);  // along cylinder.  0 to NUM_SEGS*SEG_ROWS (120)
        // reverse every other row
        if (y % 2 == 1) {
            x = (NUM_COLUMNS-1)-x;
        }

    squares_sprite* s = squares_top_sprite(x,y);
    if ( NULL == s ) {
      paint_rgb(pixels, i, squares_bg.r, squares_bg.g, squares_bg.b, alpha);
    } else {
      paint_rgb(pixels, i, s->r, s->g, s->b, alpha);
    }
  }

  if (p->frame != last_frame) {
    squares_move_sprites(p->frame - last_frame);
  }
  last_frame = p->frame;

  copy_body_to_head(pixels, head);
  return 1;
}

void squares_init_sprites() {
  srand( time(NULL) );
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprites[i].w = rand()%SQUARES_MAX_W;
    squares_sprites[i].h = rand()%SQUARES_MAX_H;
    squares_sprites[i].z = rand()%SQUARES_MAX_Z;
    squares_sprites[i].r = rand()%256;
    squares_sprites[i].g = rand()%256;
    squares_sprites[i].b = rand()%256;
    float r = (float)rand()/(float)RAND_MAX;
    squares_sprites[i].x = r * (SQUARES_MAX_W*2+NUM_COLUMNS);
    r = (float)rand()/(float)RAND_MAX;
    squares_sprites[i].y = r * (SQUARES_MAX_H*2+NUM_ROWS);
    r = (float)(rand()-rand())/(float)RAND_MAX;
    squares_sprites[i].dx = r * SQUARES_MAX_DX;
    r = (float)(rand()-rand())/(float)RAND_MAX;
    squares_sprites[i].dy = r * SQUARES_MAX_DX;
  }
}

squares_sprite* squares_top_sprite(int x, int y) {
  squares_sprite* current = NULL;
  // test hit
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprite* s = &squares_sprites[i];
    int highest_z = 0;
    if ( x >= s->x && x < s->x+s->w && y >= s->y && y < s->y+s->h ) {
      if ( s->z > highest_z ) {
        current = s;
        highest_z = s->z;
      }
    }
  }
  
  return current;
}

void squares_move_sprites(float df) {
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprite* s = &squares_sprites[i];
    s->x += s->dx*df;
    s->y += s->dy*df;
    if (s->x < 0) {
      s->x = -s->x;
      s->dx = -s->dx;
    }
    if (s->x > NUM_COLUMNS - s->w) {
      s->x = (NUM_COLUMNS - s->w) - (s->x - (NUM_COLUMNS - s->w));
      s->dx = -s->dx;
    }
    if (s->y < 0) {
      s->y = -s->y;
      s->dy = -s->dy;
    }
    if (s->y > NUM_ROWS - s->h) {
      s->y = (NUM_ROWS - s->h) - (s->y - (NUM_ROWS - s->h));
      s->dy = -s->dy;
    }
  }
}


// "plasma", by Ka-Ping Yee ================================================

byte plasma_next_frame(pattern* p, pixel* pixels, pixel* head) {
  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 2*60*SEC, 3*SEC);
  float f = p->frame * 0.4;
  float filter = 1.0;
  float target_altitude = -100;
  float spread = 1;

  if (p->frame == 0) {
    midi_set_control_with_pickup(7, 64);
    midi_set_control_with_pickup(8, 0);
  }

  if (midi_get_control(8) > 0) {
    spread = midi_get_control_exp(7, 0.1, 1.6);
    target_altitude = midi_get_control_linear(8, -3, 3);
  }

  for (int r = 0; r < NUM_ROWS; r++) {
    for (int c = 0; c < NUM_COLUMNS; c++) {
      float altitude =
          SIN((r*0.7 - c + f*2)*2.9) +
          SIN((SIN(r*4)*40 + c*1.2 + f)*4.7) +
          SIN((c - r*0.4 - f*0.7)*2.1) *
          SIN((SIN(c*3.4 + r*0.3)*20 - f*2)*5.1);
      if (target_altitude > -100) {
        filter = 1 - fabs(altitude - target_altitude)/spread;
        if (filter < 0) filter = 0;
        paint_rgb(pixels, pixel_index(r, c), 0, 0, 0, alpha);
      }
      paint_from_palette(
          pixels, pixel_index(r, c), SPECTRUM_PALETTE,
          0.5 + altitude*0.3, alpha*filter);
    }
  }

  copy_body_to_head(pixels, head);
  return 1;
}


// "ripple", by Christopher De Vries =======================================

#define RIPPLES 10

byte ripple_next_frame(pattern* p, pixel* pixels, pixel* head) {
  static int posx[RIPPLES];
  static int posy[RIPPLES];
  static int radius[RIPPLES];
  static unsigned char red[RIPPLES];
  static unsigned char green[RIPPLES];
  static unsigned char blue[RIPPLES];
  static byte temp_pixels[NUM_PIXELS*3];

  static int nnext=0;
  static int nmax=0;
  int i,j,k;
  int distance;
  int rmin;
  int rmax;
  int pixnum;

  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 2*60*SEC, 3*SEC);
  int frame = p->frame - 3*SEC;

  for(i=0;i<9000;i++) {
    temp_pixels[i]=0;
  }

  if(frame>=0) {
    if(frame%32==0 && frame != last_frame) {
      red[nnext]=rand()%100+26;
      green[nnext]=rand()%100+26;
      blue[nnext]=rand()%100+26;
      radius[nnext]=0;
      posx[nnext]=rand()%120;
      posy[nnext]=rand()%25+50;
 
      nnext++;
      if(nnext==RIPPLES) nnext=0;
 
      if(nmax<RIPPLES) {
        nmax++;
      }
    }
 
    if (frame != last_frame) {
      for(i=0;i<nmax;i++) {
        radius[i]+=1;
        red[i]=red[i]*49/50;
        green[i]=green[i]*49/50;
        blue[i]=blue[i]*49/50;
      }
    }
 
    for(k=0;k<nmax;k++) {
      if(radius[k]/6==0) {
        rmin=0;
        rmax=radius[k]*radius[k];
      }
      else {
        rmin=radius[k]-radius[k]/6;
        rmin*=rmin;
        rmax=radius[k]+radius[k]/6;
        rmax*=rmax;
      }
 
      for(j=0;j<125;j++) {
        for(i=0;i<120;i++) {
          distance = (posx[k]-i)*(posx[k]-i)+(posy[k]-j)*(posy[k]-j);
          if(distance>=rmin && distance<=rmax) {
            pixnum = pixel_index(i,j%25)*3;
            if(255-temp_pixels[pixnum]<red[k]) {
             temp_pixels[pixnum]=255;
            }
            else {
              temp_pixels[pixnum]+=red[k];
            }
            pixnum++;
 
            if(255-temp_pixels[pixnum]<green[k]) {
             temp_pixels[pixnum]=255;
            }
            else {
              temp_pixels[pixnum]+=green[k];
            }
            pixnum++;
 
 
            if(255-temp_pixels[pixnum]<blue[k]) {
             temp_pixels[pixnum]=255;
            }
            else {
              temp_pixels[pixnum]+=blue[k];
            }
          }
        }
      }
    }
  }

  byte* t = temp_pixels;
  for(i=0;i<NUM_PIXELS;i++) {
    byte r = *(t++);
    byte g = *(t++);
    byte b = *(t++);
    paint_rgb(pixels, i, r, g, b, alpha); 
  }

  last_frame = frame;

  copy_body_to_head(pixels, head);
  return 1;
}


// "rabbit-rainbow-twist", by David Rabbit Wallace =========================

float max(float a, float b) {
    if (a > b) {return a;}
    return b;
}

float min(float a, float b) {
    if (a < b) {return a;}
    return b;
}

byte rabbit_rainbow_twist_next_frame(pattern* p, pixel* pixels, pixel* head) {
    float frame = p->frame;
    short alpha = get_alpha_or_terminate(frame, 3*SEC, 3*60*SEC, 3*SEC);

    // coordinates
    int x,y;         // x is around, y is along
    int seg;         // which trailer?

    int twisted_x;
    int is_on_bottom = 0;
    float pct1; // for color
    float pct2; // for black rings
    float pct3; // for big black sine wave
    float offset;

    // color
    int r,g,b;
    float rf, gf, bf;
    float brightness;

    float sat;
    float maxx, minn, avg;

    float black_thresh = 0.5;
    black_thresh = (SIN256(frame*0.0039/(2*M_PI))/2+0.5) *0.5+0.05;

    // color movement and thin black ring movement
    float secondsPerSpin = 2;
    float moveSpeed = 1.0 / (80*secondsPerSpin);

    float minColor = 0.1;
    float minBrightness = 0.3;

    float rFreq = 1.0 * 0.4;  // 0.3, 0.4, 0.5
    float gFreq = 1.3 * 0.4;
    float bFreq = 1.8 * 0.4;
    float aFreq = 15;

    float rSpeed =   0.9;
    float gSpeed = - 1.0;
    float bSpeed =   1.3;
    float aSpeed = - 3.0;

    // black stripe, twist, and top half / bottom half
    float twist = 1.3;
    float second_half_phase_offset = -0.5;
    int black_stripe_width = 4;  // in pixels
    //twist = sin(frame*0.01) * 0.5 + 1;

    // distortion of thin black rings
    float twirl1 = 0.25/NUM_COLUMNS; 
    float twirl2 = 0.25/NUM_COLUMNS;
    twirl1 = sin(frame*0.0143) * 0.22 + 0.25;
    twirl1 /= NUM_COLUMNS;
    twirl2 = sin(frame*0.01 + 0.123) * 0.22 + 0.25;
    twirl2 /= NUM_COLUMNS;

    for (int i = 0; i < NUM_PIXELS; i++) {
        //-------------------------------------------
        // RADIAL COORDINATES
        x = (i % NUM_COLUMNS);  // theta.  0 to 24
        y = (i / NUM_COLUMNS);  // along cylinder.  0 to NUM_SEGS*NUM_ROWS (120)
        seg = y / SEG_ROWS;
        if (y % 2 == 1) {
            x = (NUM_COLUMNS-1)-x;  // reverse every other row
        }

        twisted_x = (int)(x + y*twist) % NUM_COLUMNS;
        is_on_bottom = (twisted_x < NUM_COLUMNS/2);

        // COLOR MOVEMENT
        offset = frame * moveSpeed;
        pct1 = (y*1.0 + twisted_x*3) / (NUM_SEGS*SEG_ROWS);
        pct2 = (y*1.0) / (NUM_SEGS*SEG_ROWS);
        pct3 = pct2;

        if (is_on_bottom == 1) {
            pct1 += second_half_phase_offset;
            pct2 += second_half_phase_offset;
            pct3 += second_half_phase_offset;
        }

        // RAINBOW
        rf = SIN256(  (pct1*rFreq + offset*rSpeed + 0.000)  ) /2+0.5 *(1-minColor)+minColor;
        gf = SIN256(  (pct1*gFreq + offset*gSpeed + 0.333)  ) /2+0.5 *(1-minColor)+minColor;
        bf = SIN256(  (pct1*bFreq + offset*bSpeed + 0.666)  ) /2+0.5 *(1-minColor)+minColor;

        //rf = (rf-1.0)*1.2 + 1.0;
        //gf = (gf-1.0)*1.2 + 1.0;
        //bf = (bf-1.0)*1.2 + 1.0;

        // BLACK STRIPE
        if (   ( (twisted_x + 1           ) % NUM_COLUMNS < black_stripe_width )  ||
               ( (twisted_x + 1 + NUM_COLUMNS/2) % NUM_COLUMNS < black_stripe_width )      ) {
            brightness = 0;
        } else {
            // SMALL BRIGHTNESS PULSES
            if (is_on_bottom == 1) {
                pct2 -= x * twirl1;
            } else {
                pct2 -= x * twirl2;
            }
            brightness = SIN256(  (pct2*aFreq + offset*aSpeed)  ) /2+0.5 *(1-minBrightness)+minBrightness;
            //brightness = (brightness-0.9) * 4 + 0.9;
        }

        //-------------------------------------------

        //rf -= SIN(pct3*3.46 + offset*2) * 1.5  /2+0.5;
        //gf -= SIN(pct3*3.46 + offset*2) * 1.5  /2+0.5;
        //bf -= SIN(pct3*3.46 + offset*2) * 1.5  /2+0.5;
        //rf = max(0,rf);
        //gf = max(0,gf);
        //bf = max(0,bf);

        rf *= brightness;
        gf *= brightness;
        bf *= brightness*0.85;
        rf = max(0,rf);
        gf = max(0,gf);
        bf = max(0,bf);

        // INCREASE CONSTRAST OF THE SATURATION
        maxx = max(0.01, max(rf, max(gf, bf)));
        minn = min(rf, min(gf, bf)); 
        sat = maxx - minn;
        sat = 1 - (1-sat)*(1-sat);  // e.g. sat = sat ** 0.5;
        rf = rf * (1-sat) + (sat) * (rf - minn) / sat;
        gf = gf * (1-sat) + (sat) * (gf - minn) / sat;
        bf = bf * (1-sat) + (sat) * (bf - minn) / sat;

        //if (rf < black_thresh || gf < black_thresh || bf < black_thresh) {
        if (rf + gf + bf < black_thresh*3) {
            rf = gf = bf = 0;
        }

        // BOOKKEEPING
        r = rf * RABBIT_MAX_BRIGHT;
        g = gf * RABBIT_MAX_BRIGHT;
        b = bf * RABBIT_MAX_BRIGHT;
        if (r < 0) { r = 0; }
        if (r > RABBIT_MAX_BRIGHT) { r = RABBIT_MAX_BRIGHT; }
        if (g < 0) { g = 0; }
        if (g > RABBIT_MAX_BRIGHT) { g = RABBIT_MAX_BRIGHT; }
        if (b < 0) { b = 0; }
        if (b > RABBIT_MAX_BRIGHT) { b = RABBIT_MAX_BRIGHT; }

        paint_rgb(pixels, i, r, g, b, alpha);
    }

    copy_body_to_head(pixels, head);
    return 1;
}


// "pond", by Ka-Ping Yee ==================================================

#define POND_FRICTION_MIN_VELOCITY 0.02
#define POND_FRICTION_FORCE 10
#define POND_TICKS_PER_FRAME 10
#define POND_DUTY_CYCLE_ON 1.0
#define POND_DUTY_CYCLE_PERIOD 10.0
#define POND_TIME_SPEEDUP 2

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

byte pond_next_frame(pattern* p, pixel* pixels, pixel* head) {
  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 2*60*SEC, 3*SEC);
  int f = p->frame;

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
      short rr = ((short) (r - 128))*1.0 + 128;
      short gg = ((short) (g - 128))*1.0 + 128;
      short bb = ((short) (b - 128))*1.0 + 128;
      *(p++) = rr < 0 ? 0 : rr > 255 ? 255 : rr;
      *(p++) = gg < 0 ? 0 : gg > 255 ? 255 : gg;
      *(p++) = bb < 0 ? 0 : bb > 255 ? 255 : bb;
    }
    for (int i = 0; i < NUM_ROWS; i++) {
      for (int j = 0; j < NUM_COLUMNS; j++) {
        pond_position[i][j] = 0;
        pond_velocity[i][j] = 0;
      }
    }
  }

  if (p->frame != last_frame) {
    float duty_phase =
        t - ((int) (t / POND_DUTY_CYCLE_PERIOD) * POND_DUTY_CYCLE_PERIOD);
    if (duty_phase >= 0 && duty_phase < POND_DUTY_CYCLE_ON && f > 5*SEC) {
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
  }

  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLUMNS; j++) {
      int e = (pond_position[i][j]-pond_position[i+1][j])*400 +
          POND_ENV_MAP_SIZE*0.35;
      e = (e < 0) ? 0 : (e > POND_ENV_MAP_SIZE - 1) ?
          POND_ENV_MAP_SIZE - 1 : e;
      unsigned char* ep = POND_ENV_MAP + e*3;
      paint_rgb(pixels, i*NUM_COLUMNS + ((i % 2) ? (NUM_COLUMNS-1-j) : j),
                ep[0]*200/255, ep[1]*240/255, ep[2]*240/255, alpha);
    }
  }

  last_frame = p->frame;

  copy_body_to_head(pixels, head);
  return 1;
}


// "twinkle", by Ka-Ping Yee ================================================

#define TWINKLE_MAX_STARS 10000
int twinkle_num_stars = 0;
float twinkle_last_frame = 0;
typedef struct {
  int index;
  pixel color;
  byte value;
  float magnitude;
  float target;
  float ttl;
} twinkle_star;
twinkle_star twinkle_stars[TWINKLE_MAX_STARS];

#define TWINKLE_MAX_METEORITES 100
int twinkle_num_meteorites = 0;
typedef struct {
  int c;
  int size;
  float r, v;
  float levels[NUM_ROWS];
} twinkle_meteorite;
twinkle_meteorite twinkle_meteorites[TWINKLE_MAX_METEORITES];

void twinkle_add_meteorite() {
  twinkle_meteorite* meteorite;
  if (twinkle_num_meteorites < TWINKLE_MAX_METEORITES) {
    meteorite = &twinkle_meteorites[twinkle_num_meteorites++];
    meteorite->c = irandom(NUM_COLUMNS);
    meteorite->r = frandom(NUM_ROWS) - 10;
    meteorite->v = frandom(NUM_ROWS) + 20;
    meteorite->size = 3000*pow(10, frandom(1));
    bzero(meteorite->levels, sizeof(int)*NUM_ROWS);
  }
}

int twinkle_advance_meteorite(twinkle_meteorite* meteorite, float dt) {
  int r, dr, keep;
  float fade = pow(0.003, dt);

  for (r = 0; r < NUM_ROWS; r++) {
    meteorite->levels[r] *= fade;
  }
  keep = 0;
  for (r = 0; r < NUM_ROWS; r++) {
    dr = r - meteorite->r;
    meteorite->levels[r] += dt*meteorite->size*0.1/(0.1 + dr*dr);
    if (meteorite->levels[r] > 0.5) keep = 1;
  }
  meteorite->r += meteorite->v*dt;
  meteorite->size *= fade;
  return keep;
}

void twinkle_add_star() {
  twinkle_star* star;
  if (twinkle_num_stars < TWINKLE_MAX_STARS) {
    star = &twinkle_stars[twinkle_num_stars++];
    if (twinkle_num_stars <= 2) {
      star->index = SEG_PIXELS*9 + TAIL_LANTERN_START +
                    (random() % TAIL_LANTERN_COUNT);
      star->value = 100 + 100 / (1.0 + frandom(30)*frandom(30));
    } else if (twinkle_num_stars <= 4) {
      star->index = CRYSTAL_START + (random() % CRYSTAL_COUNT);
      star->value = 100 + 100 / (1.0 + frandom(30)*frandom(30));
    } else {
      star->index = random() % NUM_PIXELS;
      star->value = 1 + 200 / (1.0 + frandom(30)*frandom(30));
    }
    star->magnitude = 1;
    hsv_to_rgb(irandom(254), irandom(32), 255, &star->color);
  }
}

byte twinkle_next_frame(pattern* p, pixel* pixels, pixel* head) {
  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 5*60*SEC, 3*SEC);
  twinkle_star* star;
  twinkle_meteorite* meteorite;
  float meteorites_per_second = midi_get_control_exp(7, 0.004, 60);
  int stars_wanted = midi_get_control_exp(8, 8, TWINKLE_MAX_STARS);
  float twinkle_amplitude = midi_get_control_exp(23, 0.1, 1);
  float twinkle_time = midi_get_control_exp(24, 1, 0.01);
  float dt = (p->frame - twinkle_last_frame)/FPS;
  pixel canvas[NUM_PIXELS];
  int i, j, level;
  pixel* pix;

  if (p->frame == 0) {
    midi_set_control_with_pickup(6, 32);  // turn off fins
    midi_set_control_with_pickup(7, 40);
    midi_set_control_with_pickup(8, 80);
    midi_set_control_with_pickup(23, 80);
    midi_set_control_with_pickup(24, 40);
  }

  while (twinkle_num_stars < stars_wanted) {
    twinkle_add_star();
  }
  twinkle_num_stars = stars_wanted;

  float fade = pow(0.5, dt/twinkle_time);
  for (i = 0; i < twinkle_num_stars; i++) {
    star = &twinkle_stars[i];
    star->ttl -= dt;
    if (star->ttl <= 0) {
      star->target = 1 - frandom(twinkle_amplitude);
      star->ttl = twinkle_time*(1 - frandom(0.5));
    }
    star->magnitude = star->magnitude*fade + star->target*(1 - fade);
  }

  if (frandom(1) < meteorites_per_second*dt) {
    twinkle_add_meteorite();
  }
  j = 0;
  for (i = 0; i < twinkle_num_meteorites; i++) {
    twinkle_advance_meteorite(&twinkle_meteorites[i], dt/2);
    if (twinkle_advance_meteorite(&twinkle_meteorites[i], dt/2)) {
      twinkle_meteorites[j++] = twinkle_meteorites[i];
    }
  }
  twinkle_num_meteorites = j;

  bzero(canvas, sizeof(pixel)*NUM_PIXELS);
  for (i = 0; i < twinkle_num_stars; i++) {
    star = &twinkle_stars[i];
    level = star->value < 10 ? star->value : pow(star->value, star->magnitude);
    paint_rgb(canvas, star->index,
              star->color.r, star->color.g, star->color.b, level);
  }
  for (i = 0; i < twinkle_num_meteorites; i++) {
    meteorite = &twinkle_meteorites[i];
    for (j = 0; j < NUM_ROWS; j++) {
      level = clamp(meteorite->levels[j], 0, 256);
      paint_rgb(canvas, pixel_index(j, meteorite->c), 255, 255, 255, level);
    }
  }
  for (i = 0, pix = canvas; i < NUM_PIXELS; i++, pix++) {
    paint_rgb(pixels, i, pix->r, pix->g, pix->b, alpha);
  }

  twinkle_last_frame = p->frame;

  copy_body_to_head(pixels, head);

  pixel gold;
  hsv_to_rgb(12, 255, 60, &gold);
  for (i = 0; i < HEAD_PIXELS; i++) {
    if (in_interval(i, left_outer_eye_start, outer_eye_length) ||
        in_interval(i, right_outer_eye_start, outer_eye_length) ||
        in_interval(i, left_medallion_start, medallion_length) ||
        in_interval(i, right_medallion_start, medallion_length)) {
      paint_rgb(head, i, gold.r, gold.g, gold.b, alpha);
    }
  }

  return 1;
}


// "fire", by Ka-Ping Yee ================================================

#define FIRE_NUM_PIXELS NUM_ROWS
int fire_levels[FIRE_NUM_PIXELS];
int fire_next_levels[FIRE_NUM_PIXELS];

void fire_blur(int count, int* levels, int* next_levels) {
  int i, j, k;

  for (i = 0; i < count; i++) {
    j = (i + 1) % count;
    k = (i + count - 1) % count;
    next_levels[i] = (levels[i]*6 + levels[j] + levels[k])/8;
  }
  memcpy(levels, next_levels, sizeof(int)*count);
}

void fire_pop(int count, int* levels, float volume, int intensity, int limit) {
  int i, excess, delta;

  while (volume >= 1.0) {
    levels[rand() % count] += intensity;
    volume = volume - 1.0;
  }
  if (rand() % 1000 < volume * 1000) {
    levels[rand() % count] += intensity;
  }

  excess = 0;
  for (i = 0; i < count; i++) {
    excess += levels[i];
  }
  excess = (excess > limit) ? excess - limit : 0;
  if (rand() % 2) {
    for (i = 0; i < count; i++) {
      delta = excess/(count - i);
      delta = (delta > levels[i]) ? levels[i] : delta;
      levels[i] -= delta;
      excess -= delta;
    }
  } else {
    for (i = count - 1; i >= 0; i--) {
      delta = excess/(i + 1);
      delta = (delta > levels[i]) ? levels[i] : delta;
      levels[i] -= delta;
      excess -= delta;
    }
  }
}

void fire_setup(int count, int* levels) {
  int i;

  for (i = 0; i < count; i++) {
    levels[i] = rand() % 100;
  }
}

void fire_set_pixels(int count, int* levels,
                      pixel c0, pixel c1, pixel c2, pixel c3, pixel* pixels) {
  int i, l, s;

  bzero(pixels, count*sizeof(pixel));
  for (i = 0; i < count; i++) {
    l = levels[i];
    s = c0.r + c1.r * clamp(l, 0, 256)/256 +
               c2.r * clamp(l - 256, 0, 256)/256 +
               c3.r * clamp(l - 512, 0, 256)/256;
    pixels[i].r = clamp(s, 0, 255);
    s = c0.g + c1.g * clamp(l, 0, 256)/256 +
               c2.g * clamp(l - 256, 0, 256)/256 +
               c3.g * clamp(l - 512, 0, 256)/256;
    pixels[i].g = clamp(s, 0, 255);
    s = c0.b + c1.b * clamp(l, 0, 256)/256 +
               c2.b * clamp(l - 256, 0, 256)/256 +
               c3.b * clamp(l - 512, 0, 256)/256;
    pixels[i].b = clamp(s, 0, 255);
  }
}

byte fire_next_frame(pattern* p, pixel* pixels, pixel* head) {
  static pixel fire_pixels[FIRE_NUM_PIXELS];
  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 5*60*SEC, 3*SEC);
  pixel* pix;
  int i, r, c;
  static int last_dim = 0;
  pixel c0 = {20, 0, 0};
  pixel c1 = {200, 20, 0};
  pixel c2 = {40, 140, 0};
  pixel c3 = {60, 60, 60};

  if (p->frame == 0) {
    midi_set_control_with_pickup(25, last_dim = midi_get_control(1));

    fire_setup(FIRE_NUM_PIXELS, fire_levels);
    midi_set_control_with_pickup(6, 96);

    midi_set_control_with_pickup(7, 0x3c);
    midi_set_control_with_pickup(8, 0x34);

    midi_set_control_with_pickup(26, 0); // c0 hue/2
    midi_set_control_with_pickup(30, 10); // c0 value/2
    midi_set_control_with_pickup(27, 2); // c1 hue/2
    midi_set_control_with_pickup(31, 100); // c1 value/2
    midi_set_control_with_pickup(28, 36); // c2 hue/2
    midi_set_control_with_pickup(32, 70); // c2 value/2
  }

  if (midi_get_control(1) != last_dim) {
    midi_set_control_with_pickup(25, last_dim = midi_get_control(1));
  }

  // Link overall dim to controller 25 (same knob in preset 2).
  midi_set_control_with_pickup(1, last_dim = midi_get_control(25));

  hsv_to_rgb(midi_get_control(26)*2, 255, midi_get_control(30)*2, &c0);
  hsv_to_rgb(midi_get_control(27)*2, 255, midi_get_control(31)*2, &c1);
  hsv_to_rgb(midi_get_control(28)*2, 255, midi_get_control(32)*2, &c2);
  if (midi_get_control(13) > 0) {
    printf("26=%02x, 27=%02x, 28=%02x\n",
           midi_get_control(26), midi_get_control(27), midi_get_control(28));
    printf("30=%02x, 31=%02x, 32=%02x\n",
           midi_get_control(30), midi_get_control(31), midi_get_control(32));
  }

  fire_blur(FIRE_NUM_PIXELS, fire_levels, fire_next_levels);
  fire_pop(FIRE_NUM_PIXELS, fire_levels, midi_get_control_exp(8, 4, 400),
           midi_get_control_exp(7, 7, 700), 200*FIRE_NUM_PIXELS);
  fire_set_pixels(FIRE_NUM_PIXELS, fire_levels, c0, c1, c2, c3, fire_pixels);
  for (r = 0; r < NUM_ROWS; r++) {
    pix = &fire_pixels[r];
    for (c = 0; c < NUM_COLUMNS; c++) {
      paint_rgb(pixels, pixel_index(r, c), pix->r, pix->g, pix->b, alpha);
    }
  }

  copy_body_to_head(pixels, head);

  pixel black = {0, 0, 0};
  pixel gold;
  hsv_to_rgb(7, 255, 32, &gold);
  for (i = TAIL_LANTERN_START; i < TAIL_LANTERN_COUNT; i++) {
    pixels[SEG_PIXELS*9 + i] = black;
    paint_rgb(pixels + SEG_PIXELS*9, i, gold.r, gold.g, gold.b, alpha);
  }

  return 1;
}


// "diner", by Ka-Ping Yee =================================================

typedef struct {
  byte head, outer_eye, inner_eye, medallion, body1, body2, lantern; 
} tint_scheme;

#define NUM_TINT_SCHEMES 16
tint_scheme tint_schemes[16] = {
  {0, 0, 1, 2, 1, 0, 2},
  {0, 0, 1, 2, 1, 0, 0},
  {0, 0, 1, 2, 1, 2, 2},
  {0, 0, 1, 2, 1, 2, 0},
  {0, 2, 1, 2, 1, 0, 2},
  {0, 2, 1, 2, 1, 0, 0},
  {0, 2, 1, 2, 1, 2, 2},
  {0, 2, 1, 2, 1, 2, 0},
  {0, 0, 1, 1, 1, 0, 2},
  {0, 0, 1, 1, 1, 0, 0},
  {0, 0, 1, 1, 1, 2, 2},
  {0, 0, 1, 1, 1, 2, 0},
  {0, 2, 1, 1, 1, 0, 2},
  {0, 2, 1, 1, 1, 0, 0},
  {0, 2, 1, 1, 1, 2, 2},
  {0, 2, 1, 1, 1, 2, 0},
};

byte diner_next_frame(pattern* p, pixel* pixels, pixel* head) {
  int i, t;
  short alpha = get_alpha_or_terminate(p->frame, 3*SEC, 5*60*SEC, 3*SEC);
  static int last_dim = 0;

  if (p->frame == 0) {
    midi_set_control_with_pickup(17, last_dim = midi_get_control(1));
    midi_set_control_with_pickup(21, 0);

    // neon blue
    midi_set_control_with_pickup(18, 0x47);
    midi_set_control_with_pickup(22, 0x20);

    // neon pink
    midi_set_control_with_pickup(19, 0x6f);
    midi_set_control_with_pickup(23, 0x43);

    // neon gold
    midi_set_control_with_pickup(20, 0x07);
    midi_set_control_with_pickup(24, 0x40);
  }

  if (midi_get_control(1) != last_dim) {
    midi_set_control_with_pickup(17, last_dim = midi_get_control(1));
  }

  // Link overall dim to controller 17 (same knob in preset 2).
  midi_set_control_with_pickup(1, last_dim = midi_get_control(17));

  pixel tint[3];
  byte tint_alpha[3];
  int scheme = midi_get_control(21)*NUM_TINT_SCHEMES/128;
  printf("tint");
  for (i = 0; i < 3; i++) {
    printf(" - %d: %02x %02x", i,
           midi_get_control(18 + i), midi_get_control(22 + i));
    setup_tint(midi_get_control_linear(18 + i, 0, 1.1),
               midi_get_control_linear(22 + i, 0, 2), 1.0,
               &tint[i], &tint_alpha[i]);
  }
  printf("   scheme %d\n", scheme);

  tint_scheme s = tint_schemes[scheme];
    
  bzero(head, sizeof(pixel)*HEAD_PIXELS);
  for (i = 0; i < HEAD_PIXELS; i++) {
    t = s.head;
    if (in_interval(i, left_outer_eye_start, outer_eye_length) ||
        in_interval(i, right_outer_eye_start, outer_eye_length)) {
      t = s.outer_eye;
    }
    if (in_interval(i, left_inner_eye_start, inner_eye_length) ||
        in_interval(i, right_inner_eye_start, inner_eye_length)) {
      t = s.inner_eye;
    }
    if (in_interval(i, left_medallion_start, medallion_length) ||
        in_interval(i, right_medallion_start, medallion_length)) {
      t = s.medallion;
    }
    paint_rgb(head, i, tint[t].r, tint[t].g, tint[t].b,
              tint_alpha[t]*alpha/255);
  }

  bzero(pixels, sizeof(pixel)*NUM_PIXELS);
  for (i = 0; i < NUM_PIXELS; i++) {
    t = ((i / SEG_PIXELS) % 2) ? s.body2 : s.body1;
    if (in_interval(i, SEG_PIXELS*9 + TAIL_LANTERN_START, TAIL_LANTERN_COUNT)) {
      t = s.lantern;
    }
    paint_rgb(pixels, i, tint[t].r, tint[t].g, tint[t].b,
              tint_alpha[t]*alpha/255);
  }

  return 1;
}

// Pattern table ===========================================================

pattern BASE_PATTERN = {"base", base_next_frame, 0};

#define NUM_PATTERNS 12
pattern PATTERNS[] = {
  {"pond", pond_next_frame, 0, 0},
  {"rabbit-sine", rabbit_sine_next_frame, 1, 0},
  {"rabbit-rainbow-twist", rabbit_rainbow_twist_next_frame, 1, 0},
  {"plasma", plasma_next_frame, 1, 0},
  {"electric", electric_next_frame, 0, 0},
  {"ripple", ripple_next_frame, 0, 0},
  {"squares", squares_next_frame, 1, 0},
  {"swirl", swirl_next_frame, 1, 0},
  {"twinkle", twinkle_next_frame, 0, 0},
  {"fire", fire_next_frame, 0, 0},
  {"diner", diner_next_frame, 0, 0},
  {"diner", diner_next_frame, 0, 0},
};


// MIDI feedback ===========================================================

int midi_pattern_selected() {
  int i, pattern = -1;

  if (midi_clicks_finished()) {
    // Click a single button to select patterns 1 through 8.
    for (i = 0; i < 8; i++) {
      if (midi_get_click(i + 1)) {
        pattern = i;
      }
    }
    // Chord two buttons to select patterns 9 through 12.
    for (i = 0; i < 4; i++) {
      if (midi_get_click(i + 1) && midi_get_click(i + 5)) {
        pattern = i + 8;
      }
    }
    midi_clear_clicks();
  }
  return pattern;
}

void midi_show_pattern(int pattern) {
  int i;
  for (i = 0; i < 8; i++) {
    midi_set_note(i + 1, (i == pattern || i == pattern - 8 ||
                          pattern >= 8 && i == pattern - 4) ? 127 : 0);
  }
}


// Master routine ==========================================================

static pattern* curp = NULL;

void activate_pattern(pattern* p) {
  printf("\nactivating %s\n", p->name);
  next_pattern_override_start = -1;
  curp = p;
  curp->frame = 0;
}

void next_frame(int frame) {
  static long time_to_next_pattern = 5*SEC;
  static int current_pattern = -1;
  static int requested_pattern = 0;
  static int next_pattern = 0;
  static int red_thing = -10;
  static float pulses[5] = {0, 0, 0, 0, 0};
  static int pulse_mode = 0;
  int i, r, c;

  if (frame == 0) {
    init_tables();
    init_head_pixel_locations();
    midi_set_control_with_pickup(1, 56);
    midi_set_control_with_pickup(2, 0);
    midi_set_control_with_pickup(3, 0);
    midi_set_control_with_pickup(4, 64);
    midi_set_control_with_pickup(5, 64);
    midi_set_control_with_pickup(17, 0);
    midi_set_control_with_pickup(18, 0);
    midi_set_control_with_pickup(19, 0);
    midi_set_control_with_pickup(20, 0);
    midi_set_control_with_pickup(21, 0);
    midi_set_control_with_pickup(22, 0);
    midi_set_control_with_pickup(23, 0);
    midi_set_control_with_pickup(24, 0);
    requested_pattern = next_pattern = getenv("BLACK_SERPENT") ? 9 : 10;
  }

  bzero(pixels, sizeof(pixel)*NUM_PIXELS);
  bzero(head, sizeof(pixel)*HEAD_PIXELS);

  if (curp) {
    if (curp->next_frame(curp, pixels, head)) {
      float frame_rate = exp((midi_get_control(5) - 64)/32.0);
      if (midi_get_control(13) > 0) {
        if (curp->time_warp_capable) {
          frame_rate *= 0.125;
        } else {
          frame_rate = 0;
        }
      }
      if (midi_get_control(5) > 0) {
        curp->frame += frame_rate;
      }
    } else {
      printf("\nfinished %s\n", curp->name);
      curp = NULL;
    }
  } else {
    if (time_to_next_pattern) {
      midi_show_pattern(-1);
      time_to_next_pattern--;
    } else {
      activate_pattern(PATTERNS + next_pattern);
      current_pattern = next_pattern;
      time_to_next_pattern = 5*SEC + (random() % (10*SEC));
      next_pattern = (next_pattern + 1) % NUM_PATTERNS;
          /*(next_pattern + (random() % (NUM_PATTERNS - 1))) % NUM_PATTERNS;*/
    }
  }
  switch ((frame/2) % 8) {
    case 1:
      midi_show_pattern(requested_pattern); break;
    case 0:
    case 2:
      midi_show_pattern(requested_pattern == current_pattern ?
                        current_pattern : -1); break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      midi_show_pattern(current_pattern); break;
  }

  // External pulse source
  if (pulse_mode) {
    FILE* fp = fopen("/tmp/pulses", "r");
    if (fp) {
      unsigned char pdata[5];
      fread(pdata, 5, 1, fp);
      fclose(fp);
      printf("\npulse: %02x %02x %02x %02x %02x\n",
             pdata[0], pdata[1], pdata[2], pdata[3], pdata[4]);
      unlink("/tmp/pulses");
      if (pulse_mode == 1) {
        for (int i = 0; i < 5; i++) {
          if (pdata[i] >= pulses[i]) {
            pulses[i] = pdata[i];
          }
        }
      } else if (pulse_mode == 2) {
        if (pdata[0] > pulses[4]) {
          pulses[4] = pdata[0];
        }
      }
    }
  }

  if (midi_get_control(13)) { pulse_mode = 0; printf("\npulse_mode 0\n"); }
  if (midi_get_control(14)) { pulse_mode = 1; printf("\npulse_mode 1\n"); }
  if (midi_get_control(15)) { pulse_mode = 2; printf("\npulse_mode 2\n"); }

  for (int i = 0; i < 5; i++) {
    int j = 9 + i + (i == 4)*3;  // midi controls 9, 10, 11, 12, 16
    if (midi_get_control(j) > 0) {
      float value = midi_get_control_exp(j, 30, 120);
      if (pulses[i] >= value) {
        pulses[i] *= 1.05;
      } else pulses[i] = value;
    } else {
      pulses[i] *= 0.84;
      if (pulses[i] < 0.1) { pulses[i] = 0; }
    }
  }

  // Copy away the spine
  for (r = 0; r < NUM_ROWS; r++) {
    spine[r] = pixels[pixel_index(r, 12)];
  }

  float brightness[NUM_ROWS];
  float spine_brightness[NUM_ROWS];
  // control 1: dim all barrels
  // control 2: desaturate
  // control 3: spotlight size & brightness
  // control 4: spotlight position
  float dim_level = midi_get_control(1)/127.0;
  float desat_level = midi_get_control(2)/127.0;
  dim_level = dim_level*dim_level;
  int pulse_value = midi_get_control(3) + pulses[4];
  float spot_gain = (pulse_value/127.0)*5;
  float spot_size = 4000.0 / (pulse_value*0.02*pulse_value + 10);
  float spot_pos = (midi_get_control(4)/127.0)*1.4 - 0.2;
  for (int r = 0; r < NUM_ROWS; r++) {
    float dist = fabs(((float) r)/NUM_ROWS - spot_pos)*spot_size;
    brightness[r] = dim_level + spot_gain*spot_gain/(1 + dist*dist*dist*dist);
    spine_brightness[r] = 1 + spot_gain*spot_gain/(1 + dist*dist*dist*dist);
  }
  for (int i = 0; i < 4; i++) {
    if (pulses[i] > 0) {
      spot_pos = 0.2 + i*0.2;
      spot_gain = (pulses[i]/127.0)*5;
      spot_size = 4000.0 / (pulses[i]*0.02*pulses[i] + 10);
      for (int r = 0; r < NUM_ROWS; r++) {
        float dist = fabs(((float) r)/NUM_ROWS - spot_pos)*spot_size;
        brightness[r] += spot_gain*spot_gain/(1 + dist*dist*dist*dist);
        spine_brightness[r] += spot_gain*spot_gain/(1 + dist*dist*dist*dist);
      }
    }
  }
  spot_pos = 0;
  for (int r = 0; r < NUM_ROWS; r++) {
    float gain = brightness[r];
    for (int c = 0; c < NUM_COLUMNS; c++) {
      pixel* p = pixels + pixel_index(r, c);
      float red = p->r, green = p->g, blue = p->b;
      float white = 0.3*red + 0.59*green + 0.11*blue;
      red = white*desat_level + red*(1 - desat_level);
      green = white*desat_level + green*(1 - desat_level);
      blue = white*desat_level + blue*(1 - desat_level);
      float x = gain * (red + (gain > 1 ? gain - 1 : 0));
      p->r = x > 255 ? 255 : x;
      x = gain * (green + (gain > 1 ? gain - 1 : 0));
      p->g = x > 255 ? 255 : x;
      x = gain * (blue + (gain > 1 ? gain - 1 : 0));
      p->b = x > 255 ? 255 : x;
    }
    gain = spine_brightness[r];
    pixel* p = spine + r;
    float x = gain * (p->r + (gain > 1 ? gain - 1 : 0));
    p->r = x > 255 ? 255 : x;
    x = gain * (p->g + (gain > 1 ? gain - 1 : 0));
    p->g = x > 255 ? 255 : x;
    x = gain * (p->b + (gain > 1 ? gain - 1 : 0));
    p->b = x > 255 ? 255 : x;
  }

  for (int i = 0; i < HEAD_PIXELS; i++) {
    pixel* p = &head[i];
    p->r *= dim_level;
    p->g *= dim_level;
    p->b *= dim_level;
  }

  i = midi_pattern_selected();
  if (i >= 0) {
    if (curp) {
      // Start fading out the current pattern.
      next_pattern_override_start = curp->frame;
    }
    time_to_next_pattern = 0;
    next_pattern = i;
    requested_pattern = i;
  }

  if (red_thing >= -6) {
    for (int r = 0; r < NUM_ROWS; r++) {
      float dr = red_thing - r;
      float d = 6*6 - dr*dr;
      if (d > 0) {
        short alpha = sqrt(d)*256/6;
        for (int c = 0; c < NUM_COLUMNS; c++) {
          paint_rgb(pixels, pixel_index(r, c), 255, 0, 0, alpha);
        }
      }
    }
    red_thing++;
    if (red_thing > NUM_ROWS) {
      red_thing = -10;
    }
  }

  put_head_pixels((byte*) head, HEAD_PIXELS);
  put_spine_pixels((byte*) spine, NUM_ROWS);

  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, (byte*) (pixels + s*SEG_PIXELS), SEG_PIXELS);
  }

  if (strcmp(get_button_sequence(), "abxbx") == 0) {
    if (curp) {
      next_pattern_override_start = curp->frame;
    }
    time_to_next_pattern = 0;
    clear_button_sequence();
  }

  if (strcmp(get_button_sequence(), "ababxxx") == 0) {
    red_thing = -6;
    clear_button_sequence();
  }
}
