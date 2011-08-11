// "pond", by David Rabbit Wallace

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

// this scales the brightness of all the pixels.  255 is default
#define MAX_BRIGHT 255

#define NSEGS 10
#define AROUND 25
#define LONG 12
unsigned char pixels[3*AROUND*LONG*NSEGS];

#define PI 3.14159
#define TWOPI (2*3.14159)

void next_frame(int frame) {
    int r,g,b,temp;

    // coordinates
    int x,y;         // x is around, y is along
    float alt;       // altitude of each point, from -1 to 1
    int seg;         // which trailer?

    // scalar field values
    int spin1;
    int spin2;
    int spin3;

    int thickness1 = 2;
    int thickness2 = 2;
    int thickness3 = 2;
    float twist1 = 0.05;
    float twist2 = 0.04;
    float twist3 = 0.03;

    // movement and look
    float wavelength_small = 12.0;
    float wavelength_large = 64.0;
    float time_offset_small = -frame*0.5;  // larger number == faster movement
    float time_offset_large = frame*0.9;
    float black_stripe_width = 0.8; // width of black stripe between colors

    spin1 = spin2 = spin3 = 0;

    for (int i = 0; i < 300*NSEGS; i++) {
        //-------------------------------------------
        // RADIAL COORDINATES
        x = (i % AROUND);  // theta.  0 to 24
        y = (i / AROUND);  // along cylinder.  0 to NSEGS*LONG (120)
        seg = y / LONG;
        // reverse every other row
        if (y % 2 == 1) {
            x = (AROUND-1)-x;
        }

        // rotate 90 degrees to one side
        //x = (x + AROUND/4) % AROUND;

        // twist like a candy cane
        //x = (x + y*0.6);
        //x = x % AROUND;

        // spin like a rolling log
        //x = (x + frame) % AROUND;

        if (x == 0) {
            // mirror y around middle of serpent
            if (y > NSEGS*LONG/2) {
                y = -y + NSEGS*LONG;
            }

            // brighter in the middle
            thickness1 = y / 20 + 1;
            thickness2 = y / 20 + 1;
            thickness2 = 4 - thickness2;
            thickness3 = y / 20 + 1;
        }

        //-------------------------------------------
        // SPIN AND COLORS

        r = g = b = 0;

        if (x == 0) {
            spin1 = y * frame * twist1 - frame;
            spin1 = spin1 % AROUND;
            if (spin1 < 0) { spin1 += AROUND; }
        }
        if ((abs(spin1 - x) < thickness1) || (abs( (spin1+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness1)) {
            r += 255;
        }
        spin1 = (spin1 + 2) % AROUND;
        if ((abs(spin1 - x) < thickness1) || (abs( (spin1+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness1)) {
            g += 55;
        }
        spin1 = (spin1 + 2) % AROUND;
        if ((abs(spin1 - x) < thickness1) || (abs( (spin1+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness1)) {
            b += 255;
        }
        spin1 = (spin1 - 4) % AROUND;



        if (x == 0) {
            spin2 = y * frame * twist2 - frame;
            spin2 = spin2 % AROUND;
            if (spin2 < 0) { spin2 += AROUND; }
        }
        if ((abs(spin2 - x) < thickness2) || (abs( (spin2+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness2)) {
            r += 255;
        }
        spin2 = (spin2 + 2) % AROUND;
        if ((abs(spin2 - x) < thickness2) || (abs( (spin2+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness2)) {
            g += 255;
        }
        spin2 = (spin2 + 2) % AROUND;
        if ((abs(spin2 - x) < thickness2) || (abs( (spin2+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness2)) {
            b += 55;
        }
        spin2 = (spin2 - 4) % AROUND;


/*
        if (x == 0) {
            spin3 = y * frame * twist3 - frame;
            spin3 = spin3 % AROUND;
            if (spin3 < 0) { spin3 += AROUND; }
        }
        if ((abs(spin3 - x) < thickness3) || (abs( (spin3+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness3)) {
            r += 0;
            g += 255;
            b += 155;
        }
        //spin3 = (spin3 + 2) % AROUND;
        //if ((abs(spin3 - x) < thickness3) || (abs( (spin3+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness3)) {
        //    g += 55;
        //}
        //spin3 = (spin3 + 2) % AROUND;
        //if ((abs(spin3 - x) < thickness3) || (abs( (spin3+AROUND/2)%AROUND - (x+AROUND/2)%AROUND ) < thickness3)) {
        //    b += 55;
        //}
        //spin3 = (spin3 - 4) % AROUND;
*/

        //-------------------------------------------
        // STORE RESULT
        if (r < 0) { r = 0; }
        if (r > MAX_BRIGHT) { r = MAX_BRIGHT; }
        if (g < 0) { g = 0; }
        if (g > MAX_BRIGHT) { g = MAX_BRIGHT; }
        if (b < 0) { b = 0; }
        if (b > MAX_BRIGHT) { b = MAX_BRIGHT; }
        pixels[i*3] = r;
        pixels[i*3 + 1] = g;
        pixels[i*3 + 2] = b;
    }
    put_pixels(0, pixels, AROUND*LONG*NSEGS);
}


