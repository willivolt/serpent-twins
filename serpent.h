#define red(pixels, index) (pixels)[(index)*3]
#define green(pixels, index) (pixels)[(index)*3 + 1]
#define blue(pixels, index) (pixels)[(index)*3 + 2]
#define set_rgb(pixels, index, r, g, b) { \
  unsigned char* p = pixels + (index)*3; \
  *(p++) = r; \
  *(p++) = g; \
  *(p++) = b; \
}

void put_pixels(int segment, unsigned char* pixels, int n);
void next_frame(int frame);
