/* Serpent main routine, for Chumby. */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include "total_control.h"
#include "serpent.h"

#define FRAME_MS 50

void put_pixels(int segment, byte* pixels, int n) {
  if (segment == 0) {
    tcl_put_pixels(pixels, n);
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
  tcl_init();
  while (1) {
    next_frame(f++);
    next_frame_time += FRAME_MS;
    now = get_milliseconds();
    printf("frame %d (%.1f fps)\r", f, f*1000.0/(now - start_time));
    fflush(stdout);
    while (now < next_frame_time) {
      now = get_milliseconds();
    }
  }
}
