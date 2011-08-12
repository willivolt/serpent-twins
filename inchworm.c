// "sparkle", by Ka-Ping Yee

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
#include "spectrum.pal"

#define REV 2*M_PI

byte pixels[NUM_ROWS*NUM_COLUMNS*3];

void next_frame(int f) {
    float t = -f*REV/40;
    for (int r = 0; r < NUM_ROWS; r++) {
        float u = r/5.0;
        float v = r/20.0;
        float w = sin(u*REV + sin(v + t)*12);
        float p = (pow(2, pow(2, w)) - 1.0)/3.0;
        p = (0.5 + p*0.4);
        int red = 127;
        int green = p*255;
        int blue = 127;
        if (p < 0.6) { red = green = 0; p = 0;}
        for (int c = 0; c < NUM_COLUMNS; c++) {
            set_rgb(pixels, pixel_index(r, c), red, green, blue);
            set_from_palette(pixels, pixel_index(r, c), SPECTRUM_PALETTE, p);
        }
    }
    for (int s = 0; s < 10; s++) {
        put_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
    }
}
