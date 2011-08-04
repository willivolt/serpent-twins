// "plasma", by Ka-Ping Yee

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

#include "serpent.h"
#include <math.h>
#include "spectrum.pal"
#include "sunset.pal" 
#define PALETTE SPECTRUM_PALETTE

float sin_table[256];
byte pixels[NUM_PIXELS*3];

#define SIN(n) sin_table[((int) (n)) & 0xff]

void next_frame(int f) {
  if (f == 0) {
    for (int i = 0; i < 256; i++) {
      sin_table[i] = sin(i/256.0*2*M_PI);
    }
  }

  for (int r = 0; r < NUM_ROWS; r++) {
    for (int c = 0; c < NUM_COLUMNS; c++) {
      float altitude =
          SIN((r*0.7 - c + f*2)*2.9) +
          SIN((SIN(r*4)*40 + c*1.2 + f)*4.7) +
          SIN((c - r*0.4 - f*0.7)*2.1) *
          SIN((SIN(c*3.4 + r*0.3)*20 - f*2)*5.1);
      set_from_palette(pixels, pixel_index(r, c), PALETTE, 0.5 + altitude*0.3);
    }
  }

  for (int s = 0; s < NUM_SEGS; s++) {
    put_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
  }
}
