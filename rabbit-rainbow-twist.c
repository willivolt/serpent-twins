// "rabbit-rainbow-twist", by David Rabbit Wallace

// Let this one run for a while; it takes about 2 minutes for
// it to cycle through bright mode and dark mode.

// Copyright 2011 David Rabbit Wallace
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
#include "serpent.h"

float sin_table[256];
#define SIN(n) sin_table[((int) ((n)*256)) & 0xff]
#define COS(n) sin_table[(((int) ((n)*256)) + 64) & 0xff]

// this scales the brightness of all the pixels.  255 is default
#define RABBIT_MAX_BRIGHT 255

unsigned char pixels[3*NUM_PIXELS];

float max(float a, float b) {
    if (a > b) {return a;}
    return b;
}

float min(float a, float b) {
    if (a < b) {return a;}
    return b;
}



void next_frame(int frame) {
    // PREPARE SIN TABLE
    if (frame == 0) {
        for (int i = 0; i < 256; i++) {
            sin_table[i] = sin(i/256.0*2*M_PI);
        }
    }

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
    black_thresh = (SIN(frame*0.0039/(2*M_PI))/2+0.5) *0.5+0.05;

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
        rf = SIN(  (pct1*rFreq + offset*rSpeed + 0.000)  ) /2+0.5 *(1-minColor)+minColor;
        gf = SIN(  (pct1*gFreq + offset*gSpeed + 0.333)  ) /2+0.5 *(1-minColor)+minColor;
        bf = SIN(  (pct1*bFreq + offset*bSpeed + 0.666)  ) /2+0.5 *(1-minColor)+minColor;

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
            brightness = SIN(  (pct2*aFreq + offset*aSpeed)  ) /2+0.5 *(1-minBrightness)+minBrightness;
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
        pixels[i*3] = r;
        pixels[i*3 + 1] = g;
        pixels[i*3 + 2] = b;
    }
    for (int s = 0; s < NUM_SEGS; s++) {
      put_segment_pixels(s, pixels + s*SEG_PIXELS*3, SEG_PIXELS);
    }
    put_head_pixels((byte*) pixels, HEAD_PIXELS);
}



