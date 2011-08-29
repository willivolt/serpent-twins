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
#include <time.h>
#include "serpent.h"
#include "spectrum.pal"
#include "sunset.pal" 


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


// Common initialization ===================================================

byte ease[257];
#define EASE(frame, period) ease[((frame) << 8) / (period)]

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

  if(frame>=0) {
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


// "squares", by Brian Schantz =============================================

#define SQUARES_NUM_SPRITES 90
#define SQUARES_MAX_W 16      // max width (around circ.) of a squares_sprite
#define SQUARES_MAX_H 24      // max height (length of serpent) of a squares_sprite
#define SQUARES_MAX_Z 100     // max # of Z-levels (could improve this by making sure no two squares_sprites are on the same Z-level)
#define SQUARES_MAX_DX 2.0      // max X-axis movement in pix/frame
#define SQUARES_MAX_DY 7.5      // max Y-axis movement in pix/frame
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
void squares_move_sprites(void);

squares_sprite squares_sprites[SQUARES_NUM_SPRITES];
squares_color squares_bg;
int squares_blinktimer;

byte squares_next_frame(pattern* p, pixel* pixels) {
  short alpha = get_alpha_or_terminate(p->frame, 4*SEC, 60*SEC, 4*SEC);
  int frame = p->frame;

  if (frame == 0) {
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
  squares_blinktimer--;
      
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
  squares_move_sprites();

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

void squares_move_sprites(void) {
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprite* s = &squares_sprites[i];
    if ( s->x < 0 || s->x > NUM_COLUMNS ) {
      s->dx *= -1;
    }
    s->x += s->dx;

    if ( s->y < 0 || s->y > NUM_ROWS ) {
      s->dy *= -1;
    }
    s->y += s->dy;
  }
}


// "plasma", by Ka-Ping Yee ================================================

byte plasma_next_frame(pattern* p, pixel* pixels) {
  short alpha = get_alpha_or_terminate(p->frame, 40*SEC, 60*SEC, 10*SEC);
  float f = p->frame * 0.1;

  for (int r = 0; r < NUM_ROWS; r++) {
    for (int c = 0; c < NUM_COLUMNS; c++) {
      float altitude =
          SIN((r*0.7 - c + f*2)*2.9) +
          SIN((SIN(r*4)*40 + c*1.2 + f)*4.7) +
          SIN((c - r*0.4 - f*0.7)*2.1) *
          SIN((SIN(c*3.4 + r*0.3)*20 - f*2)*5.1);
      paint_from_palette(
          pixels, pixel_index(r, c), SPECTRUM_PALETTE,
          0.5 + altitude*0.3, alpha);
    }
  }

  return 1;
}


// "ripple", by Christopher De Vries =======================================

#define RIPPLES 10

byte ripple_next_frame(pattern* p, pixel* pixels) {
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

  short alpha = get_alpha_or_terminate(p->frame, 2*SEC, 60*SEC, 10*SEC);
  int frame = p->frame - 5*SEC;

  for(j=0;i<9000;i++) {
    temp_pixels[i]=0;
  }

  if(frame>=0) {
    if(frame%32==0) {
      red[nnext]=rand()%100+56;
      green[nnext]=rand()%100+56;
      blue[nnext]=rand()%100+56;
      radius[nnext]=0;
      posx[nnext]=rand()%120;
      posy[nnext]=rand()%25+50;
 
      nnext++;
      if(nnext==RIPPLES) nnext=0;
 
      if(nmax<RIPPLES) {
        nmax++;
      }
    }
 
    for(i=0;i<nmax;i++) {
      radius[i]+=1;
      red[i]=red[i]*49/50;
      green[i]=green[i]*49/50;
      blue[i]=blue[i]*49/50;
 
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

  return 1;
}


// "rabbit-rainbow-twist", by David Rabbit Wallace =========================

inline float max(float a, float b) {
    if (a > b) {return a;}
    return b;
}

inline float min(float a, float b) {
    if (a < b) {return a;}
    return b;
}

byte rabbit_rainbow_twist_next_frame(pattern* p, pixel* pixels) {
    int frame = p->frame;
    short alpha = get_alpha_or_terminate(frame, 10*SEC, 3*60*SEC, 10*SEC);

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

    return 1;
}


// Pattern table ===========================================================

pattern BASE_PATTERN = {"base", base_next_frame, 0};

#define NUM_PATTERNS 7
pattern PATTERNS[] = {
  {"rabbit-rainbow-twist", rabbit_rainbow_twist_next_frame, 0},
  {"ripple", ripple_next_frame, 0},
  {"plasma", plasma_next_frame, 0},
  {"squares", squares_next_frame, 0},
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
  static long time_to_next_pattern = 30*SEC;
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
      time_to_next_pattern = 5*SEC + (random() % (3*60*SEC));
      next_pattern =
          (next_pattern + (random() % (NUM_PATTERNS - 1))) % NUM_PATTERNS;
    }
  }

  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, (byte*) (pixels + s*SEG_PIXELS), SEG_PIXELS);
  }

  if (strcmp(get_button_sequence(), "abxbx") == 0) {
    curp = NULL;
    time_to_next_pattern = 0;
    clear_button_sequence();
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
