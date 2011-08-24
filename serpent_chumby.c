/* Serpent main routine, for Chumby. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include "total_control.h"
#include "serpent.h"

#define FRAME_MS 50

static byte head[(1 + HEAD_PIXELS)*3];
static byte segments[NUM_SEGS][(1 + SEG_PIXELS)*3];
static byte* strand_ptrs[1 + NUM_SEGS] = {
  head,
  segments[0],
  segments[1],
  segments[2],
  segments[3],
  segments[4],
  segments[5],
  segments[6],
  segments[7],
  segments[8],
  segments[9]
};

static int longest_sequence = 0;

static byte diagnostic_colours[11][3] = {
  {255, 255, 255},  // white for head
  {32, 0, 0},  // red for segment 1
  {32, 8, 0},  // orange
  {32, 32, 0},  // yellow
  {0, 32, 0},  // green
  {0, 32, 32},  // cyan
  {0, 8, 32},  // ice
  {0, 0, 32},  // blue
  {8, 0, 32},  // violet
  {32, 0, 32},  // lavender
  {32, 0, 8}  // pink for segment 10
};

static int toggle = 0;

void put_head_pixels(byte* pixels, int n) {
  head[0] = diagnostic_colours[0][0];  // diagnostic
  head[1] = diagnostic_colours[0][1];
  head[2] = diagnostic_colours[0][2];
  if (toggle % 3) {
    head[0] = head[1] = head[2] = 0;
  }
  toggle++;
  memcpy(head + 3, pixels, n*3);  // skip first pixel
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

void put_segment_pixels(int segment, byte* pixels, int n) {
  segments[segment][0] = diagnostic_colours[segment + 1][0];  // diagnostic
  segments[segment][1] = diagnostic_colours[segment + 1][1];
  segments[segment][2] = diagnostic_colours[segment + 1][2];
  if (toggle % 5) {
    segments[segments][0] = 0;
    segments[segments][1] = 0;
    segments[segments][2] = 0;
  }
  toggle++;
  memcpy(segments[segment] + 3, pixels, n*3);  // skip first pixel
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

int read_button(int b) {
  return tcl_read_button(b);
}

int get_milliseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

int main(int argc, char* argv[]) {
  int f = 0;
  int start_time = get_milliseconds();
  int next_frame_time = start_time;
  int now;
  int s, i;

  bzero(head, (1 + HEAD_PIXELS)*3);
  bzero(segments, NUM_SEGS*(1 + SEG_PIXELS)*3);
  tcl_init();
  while (1) {
    longest_sequence = 0;
    next_frame(f++);
    tcl_put_pixels_multi(strand_ptrs, 1 + NUM_SEGS, longest_sequence + 1);
    next_frame_time += FRAME_MS;
    now = get_milliseconds();
    printf("frame %d (%.1f fps)\r", f, f*1000.0/(now - start_time));
    fflush(stdout);
    while (now < next_frame_time) {
      now = get_milliseconds();
    }
  }
}
