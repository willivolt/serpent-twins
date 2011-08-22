#include <string.h>
#include "serpent.h"

byte head[HEAD_PIXELS*3];
byte segments[NUM_SEGS][SEG_PIXELS*3];

pixel segcolours[NUM_SEGS] = {
  {128, 0, 0},  // red
  {128, 32, 0},  // orange
  {128, 128, 0},  // yellow
  {0, 128, 0},  // green
  {0, 128, 128},  // cyan
  {0, 32, 128},  // ice
  {0, 0, 128},  // blue
  {32, 0, 128},  // violet
  {64, 0, 128},  // lavender
  {128, 0, 128}  // magenta
};

void next_frame(int frame) {
  int i, s;

  memset(head, 0, HEAD_PIXELS*3);
  memset(segments, 0, NUM_SEGS*SEG_PIXELS*3);
  for (i = 0; i < HEAD_PIXELS; i++) {
    if (i == frame % HEAD_PIXELS) {
      set_rgb(head, i, 255, 255, 255);
    } else {
      set_rgb(head, i, 1, 1, 1);
    }
  }
  for (s = 0; s < NUM_SEGS; s++) {
    for (i = 0; i < SEG_PIXELS; i++) {
      if ((frame % 20 < 10) && ((frame/20) % NUM_SEGS) == s) {
        set_rgb(segments[s], i, 0, 0, 0);
      } else if (i == s) {
        set_rgb(segments[s], i, 0, 0, 0);
      } else if (i == frame % SEG_PIXELS || i == (frame + s + 1) % SEG_PIXELS) {
        set_rgb(segments[s], i, 255, 255, 255);
      } else {
        set_pixel(segments[s], i, segcolours[s]);
      }
    }
  }

  put_head_pixels(head, HEAD_PIXELS);
  for (s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, &(segments[s][0]), SEG_PIXELS);
  }
}
