#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

// Initialize Total Control Lighting.
void tcl_init();

// Start a new pixel sequence.
void tcl_reset();

// Send a single pixel in a sequence.
void tcl_put_pixel(byte red, byte green, byte blue);

// Send an entire sequence of n pixels, given n*3 bytes of colour data.
void tcl_put_pixels(byte* pixels, int n);
