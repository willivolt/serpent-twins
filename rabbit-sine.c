#include <math.h>
#include "serpent.h"

#define NSEGS 10
#define AROUND 25
#define LONG 12
unsigned char pixels[3*AROUND*LONG*NSEGS];

void next_frame(int frame) {
    int r, g, b;
    int x,y;
    float bright,alt;
    float small_sin;
    float large_sin;
    int phase;

    float vscale = 0.7;   // 1 is full height sine wave
    float vshift = 0.00;
    float waves_per_seg = 0.5;
    float time_offset_small = -frame*0.5;
    float time_offset_large = frame*0.9;

    float black_width = 0.8;

    for (int i = 0; i < 300*NSEGS; i++) {
        x = (i % AROUND);  // theta.  0 to 24
        y = (i / AROUND);  // along cylinder.  0 to NSEGS*LONG (120)
        // reverse every other row
        if (y % 2 == 1) {
            x = (AROUND-1)-x;
        }

        alt = cos((x + frame)/24.0 * 2*3.14159)/2.0+0.5;
        alt = 1-alt;
        alt = alt*2.0-1.0; // altitude, from -1 to 1
        //if (sin((time_offset + y / 12.0) * waves_per_seg *2*3.14159)/2.0*vscale+0.5+(vshift/2.0) > alt) {
        //    bright = 0;
        //} else {
        //    bright = 1;
        //}
        small_sin = sin(((time_offset_small + y) / 6.0) * waves_per_seg *2*3.14159);
        large_sin = sin(((time_offset_large + y) / 32.0) * waves_per_seg *2*3.14159);
        bright = (small_sin*0.5 + large_sin*0.7)*0.7 + alt*0.5;


        bright = (bright*7.0) / 2.0 + 0.5 - 0.5;
        bright = bright * -1;
        bright *= 0.6;

        if (0 && (-black_width < bright) && (bright < black_width)) {
            if (bright < 0) { bright = -bright; }
            bright = bright / black_width;
            bright = 1-bright;
            bright = (bright - 0.8) / (1-0.8);
            r = bright * 1.0 * 155;
            g = bright * 0.0 * 155;
            b = bright * 2.0 * 155;
        } else {
            if (bright > 0) {
                bright = bright - black_width;
                r = bright * 0.1 * 155;
                g = bright * 0.5 * 155;
                b = bright * 2.0 * 155;
            } else {
                bright = bright * -1;
                bright = bright - black_width;
                r = bright * 2.0 * 155;
                g = bright * 0.5 * 155;
                b = bright * 0.1 * 155;
            }
        }
        if (r < 0) { r = 0; }
        if (r > 155) { r = 155; }
        if (g < 0) { g = 0; }
        if (g > 155) { g = 155; }
        if (b < 0) { b = 0; }
        if (b > 155) { b = 155; }
        pixels[i*3] = r;
        pixels[i*3 + 1] = g;
        pixels[i*3 + 2] = b;
    }
    put_pixels(0, pixels, AROUND*LONG*NSEGS);
}


