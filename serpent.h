#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

typedef unsigned char byte;

#define set_rgb(pixels, index, r, g, b) { \
  byte* p = pixels + (index)*3; \
  *(p++) = r; \
  *(p++) = g; \
  *(p++) = b; \
}

void put_pixels(int segment, byte* pixels, int n);
void next_frame(int frame);
