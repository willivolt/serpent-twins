// "text", by Ka-Ping Yee

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
#include <stdio.h>
#include "serpent.h"
#include "font.h"

unsigned char pixels[NUM_PIXELS*3];

void put_pixel(int x, int y, byte r, byte g, byte b) {
  set_rgb_rc(pixels, x, y, r, g, b);
}

font* f = NULL;

void next_frame(int x) {
  if (x == 0) {
    f = font_read("lucidatypewriter.pgm", 32, 6);
    fprintf(stderr, "%p\n", f);
  }

  char msg[2] = " ";
  for (int i = 0; i < 9; i++) {
    msg[0] = '1' + i;
    font_draw(f, msg, i*12, 13, put_pixel, 255, 255, 255);
  }

  for (int s = 0; s < 10; s++) {
    put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
}
