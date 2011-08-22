/* Serpent main routine, for Chumby. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include "total_control.h"
#include "serpent.h"

#define FRAME_MS 50

byte strands[NUM_SEGS][SEG_PIXELS*3];
byte* strand_ptrs[NUM_SEGS] = {
  &(strands[0][0]),
  &(strands[1][0]),
  &(strands[2][0]),
  &(strands[3][0]),
  &(strands[4][0]),
  &(strands[5][0]),
  &(strands[6][0]),
  &(strands[7][0]),
  &(strands[8][0]),
  &(strands[9][0])
};

int longest_sequence = 0;

void put_pixels(int segment, byte* pixels, int n) {
  memcpy(strand_ptrs[segment], pixels, n*3);
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

  bzero(strands, NUM_SEGS*SEG_PIXELS*3);
  tcl_init();
  while (1) {
    longest_sequence = 0;
    next_frame(f++);
    tcl_put_pixels_multi(strand_ptrs, NUM_SEGS, longest_sequence);
    next_frame_time += FRAME_MS;
    now = get_milliseconds();
    printf("frame %d (%.1f fps)\r", f, f*1000.0/(now - start_time));
    fflush(stdout);
    while (now < next_frame_time) {
      now = get_milliseconds();
    }
  }
}
