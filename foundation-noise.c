// "foundation", by Ka-Ping Yee

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
#include "serpent.h"

#define clamp(x) ((x) < 0 ? 0 : (x) > 255 ? 255 : (x))

typedef unsigned int word;


// noise

#define B 0x100
#define BM 0xff

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

static int p[B + B + 2];
static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];
static int start = 1;

static void init(void);

#define s_curve(t) ( t * t * (3. - 2. * t) )

#define lerp(t, a, b) ( a + t * (b - a) )

#define setup(i,b0,b1,r0,r1)\
  t = vec[i] + N;\
  b0 = ((int)t) & BM;\
  b1 = (b0+1) & BM;\
  r0 = t - (int)t;\
  r1 = r0 - 1.;

double noise1(double arg) {
  int bx0, bx1;
  float rx0, rx1, sx, t, u, v, vec[1];

  vec[0] = arg;
  if (start) {
    start = 0;
    init();
  }

  setup(0, bx0,bx1, rx0,rx1);

  sx = s_curve(rx0);

  u = rx0 * g1[ p[ bx0 ] ];
  v = rx1 * g1[ p[ bx1 ] ];

  return lerp(sx, u, v);
}

float noise2(float vec[2]) {
  int bx0, bx1, by0, by1, b00, b10, b01, b11;
  float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
  int i, j;

  if (start) {
    start = 0;
    init();
  }

  setup(0, bx0,bx1, rx0,rx1);
  setup(1, by0,by1, ry0,ry1);

  i = p[ bx0 ];
  j = p[ bx1 ];

  b00 = p[ i + by0 ];
  b10 = p[ j + by0 ];
  b01 = p[ i + by1 ];
  b11 = p[ j + by1 ];

  sx = s_curve(rx0);
  sy = s_curve(ry0);

#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

  q = g2[ b00 ] ; u = at2(rx0,ry0);
  q = g2[ b10 ] ; v = at2(rx1,ry0);
  a = lerp(sx, u, v);

  q = g2[ b01 ] ; u = at2(rx0,ry1);
  q = g2[ b11 ] ; v = at2(rx1,ry1);
  b = lerp(sx, u, v);

  return lerp(sy, a, b);
}

float noise3(float vec[3]) {
  int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
  float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
  int i, j;

  if (start) {
    start = 0;
    init();
  }

  setup(0, bx0,bx1, rx0,rx1);
  setup(1, by0,by1, ry0,ry1);
  setup(2, bz0,bz1, rz0,rz1);

  i = p[ bx0 ];
  j = p[ bx1 ];

  b00 = p[ i + by0 ];
  b10 = p[ j + by0 ];
  b01 = p[ i + by1 ];
  b11 = p[ j + by1 ];

  t  = s_curve(rx0);
  sy = s_curve(ry0);
  sz = s_curve(rz0);

#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

  q = g3[ b00 + bz0 ] ; u = at3(rx0,ry0,rz0);
  q = g3[ b10 + bz0 ] ; v = at3(rx1,ry0,rz0);
  a = lerp(t, u, v);

  q = g3[ b01 + bz0 ] ; u = at3(rx0,ry1,rz0);
  q = g3[ b11 + bz0 ] ; v = at3(rx1,ry1,rz0);
  b = lerp(t, u, v);

  c = lerp(sy, a, b);

  q = g3[ b00 + bz1 ] ; u = at3(rx0,ry0,rz1);
  q = g3[ b10 + bz1 ] ; v = at3(rx1,ry0,rz1);
  a = lerp(t, u, v);

  q = g3[ b01 + bz1 ] ; u = at3(rx0,ry1,rz1);
  q = g3[ b11 + bz1 ] ; v = at3(rx1,ry1,rz1);
  b = lerp(t, u, v);

  d = lerp(sy, a, b);

  return lerp(sz, c, d);
}

static void normalize2(float v[2]) {
  float s;

  s = sqrt(v[0] * v[0] + v[1] * v[1]);
  v[0] = v[0] / s;
  v[1] = v[1] / s;
}

static void normalize3(float v[3]) {
  float s;

  s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] = v[0] / s;
  v[1] = v[1] / s;
  v[2] = v[2] / s;
}

static void init(void) {
  int i, j, k;

  for (i = 0 ; i < B ; i++) {
    p[i] = i;

    g1[i] = (float)((random() % (B + B)) - B) / B;

    for (j = 0 ; j < 2 ; j++)
      g2[i][j] = (float)((random() % (B + B)) - B) / B;
    normalize2(g2[i]);

    for (j = 0 ; j < 3 ; j++)
      g3[i][j] = (float)((random() % (B + B)) - B) / B;
    normalize3(g3[i]);
  }

  while (--i) {
    k = p[i];
    p[i] = p[j = random() % B];
    p[j] = k;
  }

  for (i = 0 ; i < B + 2 ; i++) {
    p[B + i] = p[i];
    g1[B + i] = g1[i];
    for (j = 0 ; j < 2 ; j++)
      g2[B + i][j] = g2[i][j];
    for (j = 0 ; j < 3 ; j++)
      g3[B + i][j] = g3[i][j];
  }
}


// plasma

#include "spectrum.pal"
#include "sunset.pal" 
#define PALETTE SPECTRUM_PALETTE

float sin_table[256];
byte plasma[NUM_PIXELS*3];

#define SIN(n) sin_table[((int) (n)) & 0xff]

void next_plasma(int frame) {
  float f = frame * 0.1;
  for (int r = 0; r < NUM_ROWS; r++) {
    for (int c = 0; c < NUM_COLUMNS; c++) {
      float altitude =
          SIN((r*0.7 - c + f*2)*2.9) +
          SIN((SIN(r*4)*40 + c*1.2 + f)*4.7) +
          SIN((c - r*0.4 - f*0.7)*2.1) *
          SIN((SIN(c*3.4 + r*0.3)*20 - f*2)*5.1);
      set_from_palette(plasma, pixel_index(r, c), PALETTE, 0.5 + altitude*0.3);
    }
  }
}


// foundation

byte head[HEAD_PIXELS*3];
byte pixels[NUM_PIXELS*3];

/* hue = 0..254, sat = 0..255, val = 0..255 */
void hsv_to_rgb(byte hue, byte sat, byte val, pixel* pix) {
  byte r = 0, g = 0, b = 0, max = 0;
  byte chr = (val*sat + 255)>>8;
  byte min = val - chr;
  byte phase = hue % 85;
  byte x;

  phase = (phase >= 43) ? 85 - phase : phase;
  x = ((word) chr * phase*6 + 127) / 255;
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

int DH_OPTIONS[7] = {-127, -85, -43, 0, 43, 85, 127};

static double h1, s1, v1, dh, s2, v2;
static double goal_h1 = 0, goal_s1 = 1, goal_v1 = 1;
static double goal_dh = 0, goal_s2 = 1, goal_v2 = 1;
static double progress;
static int last_selected;
static double speed;
static double breath_phase = 0;
static double breath_speed = 0.001;
static double mood_step = 0;
static int plasma_frame = -1;

void select_goal(int f) {
  h1 = goal_h1;
  s1 = goal_s1;
  v1 = goal_v1;
  dh = goal_dh;
  s2 = goal_s2;
  v2 = goal_v2;
  goal_h1 = rand() % 256;
  goal_s1 = rand() % 185 + 70;
  goal_v1 = rand() % 255 + 1;
  goal_dh = DH_OPTIONS[rand() % 7];
  goal_s2 = rand() % 185 + 70;
  goal_v2 = rand() % 255 + 1;
  progress = 0;
  speed = 0.050 + (rand() % 200)*0.001;
  speed = speed * speed * speed * 3;
  breath_speed = 0.04;

  printf("-> %d/%d/%d, +%d/%d/%d, speed %.4lf\n",
         (int) goal_h1, (int) goal_s1, (int) goal_v1,
         (int) goal_dh, (int) goal_s2, (int) goal_v2,
         speed);

  last_selected = f;
}

void next_frame(int f) {
  if (f == 0) {
    select_goal(f);
  }
  if (f == 0) {
    for (int i = 0; i < 256; i++) {
      sin_table[i] = sin(i/256.0*2*M_PI);
    }
  }

  if (read_button(1)) {
    plasma_frame = 0;
  }
  if (plasma_frame >= 0) {
    next_plasma(plasma_frame++);
  }

  pixel hp, tp;

  byte h = h1 + progress*(goal_h1 - h1);
  byte s = s1 + progress*(goal_s1 - s1);
  byte v = v1 + progress*(goal_v1 - v1);
  float min_v = v;
  hsv_to_rgb(h, s, v, &hp);

  h = h1 + dh + progress*(goal_h1 + goal_dh - h1 - dh);
  s = s2 + progress*(goal_s2 - s2);
  v = v2 + progress*(goal_v2 - v2);
  if (v < min_v) { min_v = v; }
  hsv_to_rgb(h + 255, s, v, &tp);

  float breath_sin = sin(breath_phase);
  for (int r = 0; r < NUM_ROWS; r++) {
    float vec[2] = {mood_step, (float) r / NUM_ROWS};
    float mood = noise2(vec) * min_v/2;
    float breath = (SIN(r*128/NUM_ROWS) - 0.5) * breath_sin * min_v/2;
    float pp = 0;
    if (plasma_frame >= 0) {
      pp = plasma_frame/40.0 + (float) r*3/NUM_ROWS - 4.0;
      if (pp < 0) pp = 0;
      if (pp > 1) pp = 1;
    }
    for (int c = 0; c < NUM_COLUMNS; c++) {
      int ri = (hp.r*(NUM_ROWS - r) + tp.r*r)/NUM_ROWS + mood + breath;
      int gi = (hp.g*(NUM_ROWS - r) + tp.g*r)/NUM_ROWS + mood + breath;
      int bi = (hp.b*(NUM_ROWS - r) + tp.b*r)/NUM_ROWS + mood + breath;
      int i = pixel_index(r, c);
      if (plasma_frame >= 0) {
        ri = ri*(1-pp) + plasma[i*3]*pp;
        gi = gi*(1-pp) + plasma[i*3 + 1]*pp;
        bi = bi*(1-pp) + plasma[i*3 + 2]*pp;
      }
      set_rgb(pixels, i, clamp(ri), clamp(gi), clamp(bi));
    }
  }
  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }

  for (int i = 0; i < HEAD_PIXELS; i++) {
    if (i >  2 + 28 + 28 + 71 + 39 + 14 &&
        i <= 2 + 28 + 28 + 71 + 39 + 14 + 22 ||
        i >  2 + 28 + 28 + 71 + 39 + 14 + 22 + 13 + 12 + 6 &&
        i <= 2 + 28 + 28 + 71 + 39 + 14 + 22 + 13 + 12 + 6 + 22) {
      set_rgb(head, i, tp.r, tp.g, tp.b);
    } else {
      set_rgb(head, i, hp.r, hp.g, hp.b);
    }
  }

  breath_phase += breath_speed;
  if (breath_phase > 2*M_PI) {
    breath_phase -= 2*M_PI;
  }
  mood_step += 0.001;
  progress += (1.0 - progress)*speed;
  if (progress > 0.999) {
    if (rand() % 1000 == 0) {
      select_goal(f);
    }
  }
}
