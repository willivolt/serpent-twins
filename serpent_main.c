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

#define HEAD_PIXELS 400

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

void put_head_pixels(byte* pixels, int n) {
  memcpy(head + 3, pixels, n*3);  // leave first pixel black
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

void put_segment_pixels(int segment, byte* pixels, int n) {
  memcpy(segments[segment] + 3, pixels, n*3);  // leave first pixel black
  if (n > longest_sequence) {
    longest_sequence = n;
  }
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
