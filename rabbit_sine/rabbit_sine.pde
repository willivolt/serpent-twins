#include <math.h>



#define red(pixels, index) (pixels)[(index)*3]
#define green(pixels, index) (pixels)[(index)*3 + 1]
#define blue(pixels, index) (pixels)[(index)*3 + 2]
#define set_rgb(pixels, index, r, g, b) { \
  byte* p = pixels + (index)*3; \
  *(p++) = r; \
  *(p++) = g; \
  *(p++) = b; \
}

float sin_table[256];
#define PI 3.14159
#define TWOPI (2*3.14159)

#define SIN(n) sin_table[((int) (n*256)) & 0xff]
#define COS(n) sin_table[(((int) (n*256)) + 64) & 0xff]

#include <SPI.h>
void setup() {
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2); // fastest on Arduino
        for (int i = 0; i < 256; i++) {
            sin_table[i] = sin(i/256.0*TWOPI);
        }
}

void loop() {
  static int frame = 0;
  next_frame(frame++);
}

void put_pixels(int segment, byte* pixels, int n) {
  send_frame(0, 0, 0, 0);
  for (int i = 0; i < n; i++) {
    send_pixel(*(pixels++), *(pixels++), *(pixels++));
  }
  send_frame(0, 0, 0, 0);
}

void send_pixel(byte red, byte green, byte blue) {
  byte flag = ((red & 0xc0) >> 6) | ((green & 0xc0) >> 4) | ((blue & 0xc0) >> 2);
  send_frame(~flag, red, green, blue);
}

void send_frame(byte flag, byte red, byte green, byte blue) {
  SPI.transfer(flag);
  SPI.transfer(blue);
  SPI.transfer(green);
  SPI.transfer(red);
}











// this scales the brightness of all the pixels.  255 is default
#define MAX_BRIGHT 255

#define NSEGS 1
#define AROUND 25
#define LONG 12
unsigned char pixels[3*AROUND*LONG*NSEGS];


void next_frame(int frame) {
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
        // x = (x + AROUND/4) % AROUND;

        // twist like a candy cane
        //x = (x + y*0.6);
        //x = x % AROUND;

        // spin like a rolling log
        //x = (x + frame) % AROUND;

        //-------------------------------------------
        // ALTITUDE
        alt = -COS(   x/(AROUND-1.0) *1  );

        //-------------------------------------------
        // BRIGHTNESS

        // combine two sine waves along the length of the snake with the altitude at each point
        sin_small = SIN(   (time_offset_small + y) / wavelength_small *  1 );
        sin_large =  SIN(   (time_offset_large + y) / wavelength_large *  1 );
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
            r = bright * 1.0 * MAX_BRIGHT;
            g = bright * 0.0 * MAX_BRIGHT;
            b = bright * 2.0 * MAX_BRIGHT;
        } else {
            if (bright > 0) {
                // blue part for positive brightness
                bright = bright - black_stripe_width;
                r = bright * 0.1 * MAX_BRIGHT;
                g = bright * 0.5 * MAX_BRIGHT;
                b = bright * 2.0 * MAX_BRIGHT;
            } else {
                // orange part for negative brightness
                bright = -bright - black_stripe_width;
                r = bright * 2.0 * MAX_BRIGHT;
                g = bright * 0.5 * MAX_BRIGHT;
                b = bright * 0.1 * MAX_BRIGHT;
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



