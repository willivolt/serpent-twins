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

// Send multiple pixels in parallel, one pixel to each strand.
void tcl_put_pixel_multi(byte** pixel_ptrs, int num_strands);

// Send multiple sequences of pixels in parallel to multiple strands.
void tcl_put_pixels_multi(byte** pixel_ptrs, int num_strands, int num_pixels);

// Read a button (b = 1, 2, 3, or 4).
byte tcl_read_button(byte b);
