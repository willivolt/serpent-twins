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
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define FPS 30

#define HEAD_PIXELS 600 // number of pixels in the serpent's head

#define NUM_SEGS 10 // number of segments in the serpent
#define SEG_ROWS 12 // rows (rings) of pixels in one segment
#define NUM_ROWS (NUM_SEGS*SEG_ROWS) // rows of pixels in the entire serpent
#define NUM_COLUMNS 25 // number of pixels in one ring
#define SEG_PIXELS (SEG_ROWS*NUM_COLUMNS) // number of pixels in one segment
#define NUM_PIXELS (NUM_SEGS*SEG_PIXELS) // number of pixels in entire serpent
#define LID_PIXELS 9 // number of pixels in one segment's lid
#define FIN_PIXELS 9 // number of pixels in one segment's fins

#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

#ifndef TYPEDEF_PIXEL
#define TYPEDEF_PIXEL
typedef struct { byte r, g, b; } pixel;
#endif

// finds the index of a pixel
#define pixel_index(row, col) \
   ((row)*NUM_COLUMNS + (((row) % 2) ? (NUM_COLUMNS - 1 - (col)) : (col)))

// sets the colour of a single pixel
#define set_rgb(pixels, index, r, g, b) { \
  byte* __p = (pixels) + (index)*3; \
  *(__p++) = r; \
  *(__p++) = g; \
  *(__p++) = b; \
}

// sets the colour of a single pixel
#define set_pixel(pixels, index, pix) { \
  byte* __p = (byte*) (&pixels[index]); \
  *(__p++) = pix.r; \
  *(__p++) = pix.g; \
  *(__p++) = pix.b; \
}

// sets a single pixel by row and column
#define set_rgb_rc(pixels, row, col, r, g, b) \
    set_rgb(pixels, pixel_index(row, col), r, g, b)

// sets a single pixel by row and column
#define set_pixel_rc(pixels, row, col, pix) \
    set_pixel(pixels, pixel_index(row, col), pix)

// maps f (a float from 0.0 to 1.0) to a colour in a given palette
#define palette_index(palette, f) ((int) (((f) - floor(f))*palette##_SIZE))

// sets the colour of a pixel from a palette
#define set_from_palette(pixels, pixel_index, palette, f) { \
  byte* __s = (palette) + palette_index(palette, f)*3; \
  byte* __d = (pixels) + (pixel_index)*3; \
  *(__d++) = *(__s++); \
  *(__d++) = *(__s++); \
  *(__d++) = *(__s++); \
}

// limits a value to an interval
#define clamp(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

void put_head_pixels(byte* pixels, int n);
void put_segment_pixels(int segment, byte* pixels, int n);
void put_fin_pixels(byte* pixels, int n);
void put_spine_pixels(byte* pixels, int n);
void next_frame(int frame);
int read_button(char b);  // 'a', 'b', 'x', or 'y'
const char* get_button_sequence();
void clear_button_sequence();
int accel_right();
int accel_forward();
