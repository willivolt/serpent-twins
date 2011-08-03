#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define NUM_SEGS 10 // number of segments in the serpent
#define SEG_ROWS 12 // rows (rings) of pixels in one segment
#define NUM_ROWS (NUM_SEGS*SEG_ROWS) // rows of pixels in the entire serpent
#define NUM_COLUMNS 25 // number of pixels in one ring
#define SEG_PIXELS (SEG_ROWS*NUM_COLUMNS) // number of pixels in one segment
#define NUM_PIXELS (NUM_SEGS*SEG_PIXELS) // number of pixels in entire serpent

typedef unsigned char byte;

#define pixel_index(row, col) \
   ((row)*NUM_COLUMNS + (((row) % 2) ? (NUM_COLUMNS - 1 - (col)) : (col)))

#define set_rgb(pixels, index, r, g, b) { \
  byte* __p = (pixels) + (index)*3; \
  *(__p++) = r; \
  *(__p++) = g; \
  *(__p++) = b; \
}

#define palette_index(palette, f) ((int) (((f) - floor(f))*palette##_SIZE))

#define set_from_palette(pixels, pixel_index, palette, f) { \
  byte* __s = (palette) + palette_index(palette, f)*3; \
  byte* __d = (pixels) + (pixel_index)*3; \
  *(__d++) = *(__s++); \
  *(__d++) = *(__s++); \
  *(__d++) = *(__s++); \
}

void put_pixels(int segment, byte* pixels, int n);
void next_frame(int frame);
