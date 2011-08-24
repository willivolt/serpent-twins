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
  {128, 0, 128},  // lavender
  {128, 0, 32}  // pink
};

void flash_number(byte* pixels, int i, int step, int number) {
  byte sequence[30];
  int j, k = 0;
  
  memset(sequence, 0, 30);
  for (j = 0; j < (number/100); j++) {
    sequence[k++] = 1;
  }
  k++;
  for (j = 0; j < ((number/10) % 10); j++) {
    sequence[k++] = 2;
  }
  k++;
  for (j = 0; j < (number % 10); j++) {
    sequence[k++] = 3;
  }
  
  set_rgb(pixels, i, 1, 1, 1);
  if (step % 5 > 2) {
    switch (sequence[step/5]) {
      case 1:
        set_rgb(pixels, i, 255, 0, 0); break;
      case 2:
        set_rgb(pixels, i, 0, 255, 0); break;
      case 3:
        set_rgb(pixels, i, 0, 0, 255); break;
    }
  }
}

void next_frame(int frame) {
  int i, s;
  int step = frame % (HEAD_PIXELS + 300);
  byte b;

  memset(head, 0, HEAD_PIXELS*3);
  memset(segments, 0, NUM_SEGS*SEG_PIXELS*3);

  if (step < 300) {
    for (i = 0; i < HEAD_PIXELS; i++) {
      flash_number(head, i, step % 150, i);
    }
    for (s = 0; s < NUM_SEGS; s++) {
      for (i = 0; i < SEG_PIXELS; i++) {
        flash_number(segments[s], i, step % 150, i + 1);
      }
    }
  } else if (step > 300) {
    step -= 300;
    for (i = 0; i < HEAD_PIXELS; i++) {
      if (i == step) { 
        set_rgb(head, i, 255, 255, 255);
      } else {
        set_rgb(head, i, 1, 1, 1);
      }
    }
    for (s = 0; s < NUM_SEGS; s++) {
      for (i = 0; i < SEG_PIXELS; i++) {
        if ((step % 20 < 10) && ((step/20) % NUM_SEGS) == s) {
          set_rgb(segments[s], i, 0, 0, 0);
        } else if (i == s + 1) {  // turn off pixel 1 on segment 0, etc.
          set_rgb(segments[s], i, 0, 0, 0);
        } else if (i == step || i == step + s + 1) {
          set_rgb(segments[s], i, 255, 255, 255);
        } else {
          set_pixel(segments[s], i, segcolours[s]);
        }
      }
    }
  }

  for (b = 1; b <= 4; b++) {
    if (read_button(b)) {
      set_rgb(head, b, 255, 255, 255);
      for (s = 0; s < NUM_SEGS; s++) {
        set_rgb(segments[s], b, 255, 255, 255);
      }
    }
  }

  put_head_pixels(head, HEAD_PIXELS);
  for (s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, &(segments[s][0]), SEG_PIXELS);
  }
}
