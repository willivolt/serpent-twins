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
  {128, 0, 0},  // red for segment 1
  {128, 32, 0},  // orange
  {128, 128, 0},  // yellow
  {0, 128, 0},  // green
  {0, 128, 128},  // cyan
  {0, 32, 128},  // ice
  {0, 0, 128},  // blue
  {32, 0, 128},  // violet
  {128, 0, 128},  // lavender
  {128, 0, 32}  // pink for segment 10
};

static int toggle = 0;
static int diagnostic_disco = 0;

void put_head_pixels(byte* pixels, int n) {
  if (diagnostic_disco) {
    head[0] = diagnostic_colours[0][0];  // diagnostic
    head[1] = diagnostic_colours[0][1];
    head[2] = diagnostic_colours[0][2];
    if (toggle % 3) {
      head[0] = head[1] = head[2] = 0;
    }
    toggle++;
  }
  memcpy(head + 3, pixels, n*3);  // skip first pixel
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

void put_segment_pixels(int segment, byte* pixels, int n) {
  if (diagnostic_disco) {
    segments[segment][0] = diagnostic_colours[segment + 1][0];  // diagnostic
    segments[segment][1] = diagnostic_colours[segment + 1][1];
    segments[segment][2] = diagnostic_colours[segment + 1][2];
    if (toggle % 5) {
      segments[segment][0] = 0;
      segments[segment][1] = 0;
      segments[segment][2] = 0;
    }
    toggle++;
  }
  memcpy(segments[segment] + 3, pixels, n*3);  // skip first pixel
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

int read_button(int button) {
  return tcl_read_button(button);
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
  int clock_delay = 50;

  bzero(head, (1 + HEAD_PIXELS)*3);
  bzero(segments, NUM_SEGS*(1 + SEG_PIXELS)*3);
  tcl_init();
  tcl_set_clock_delay(clock_delay);
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
    if (read_button('a') && read_button('b') && read_button('x')) {
      clock_delay++;
      tcl_set_clock_delay(++clock_delay);
      diagnostic_disco = 1;
    }
    if (read_button('a') && read_button('b') && read_button('y')) {
      tcl_set_clock_delay(--clock_delay);
      diagnostic_disco = 0;
    }
  }
}
