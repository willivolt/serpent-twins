#include <string.h>
#include "serpent.h"
#include "total_control.h"

byte head[HEAD_PIXELS*3];
byte segments[NUM_SEGS][SEG_PIXELS*3];

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
  memset(head, 0, HEAD_PIXELS*3);
  memset(segments, 0, NUM_SEGS*SEG_PIXELS*3);
  int flash = (frame % 8) < 2;
  int count = (frame % 100)/8;
  int s, i;

  for (s = 0; s < HEAD_PIXELS; s++) {
    set_rgb(head, s, flash ? 0 : 255, flash ? 255 : 0, 0);
  }

  for (s = 0; s < NUM_SEGS; s++) {
    if (count <= s && flash) {
      for (i = 0; i < SEG_PIXELS; i++) {
        set_rgb(segments[s], i, 0, 255, 0);
      }
    } else {
      for (i = 0; i < SEG_PIXELS; i++) {
        set_rgb(segments[s], i, 255, 0, 0);
      }
    }
  }
  
  put_head_pixels(head, HEAD_PIXELS);
  for (s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, &(segments[s][0]), SEG_PIXELS);
  }
}
