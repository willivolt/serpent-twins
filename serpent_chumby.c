/* Serpent main routine, for Chumby. */

#define _SVID_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include "total_control.h"
#include "serpent.h"

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
  head[0] = head[1] = head[2] = 0;
  if (diagnostic_disco && (toggle % 3)) {
    head[0] = diagnostic_colours[0][0];  // diagnostic
    head[1] = diagnostic_colours[0][1];
    head[2] = diagnostic_colours[0][2];
  }
  toggle++;
  memcpy(head + 3, pixels, n*3);  // skip first pixel
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

void put_segment_pixels(int segment, byte* pixels, int n) {
  segments[segment][0] = segments[segment][1] = segments[segment][2] = 0;
  if (diagnostic_disco && (toggle % 5)) {
    segments[segment][0] = diagnostic_colours[segment + 1][0];  // diagnostic
    segments[segment][1] = diagnostic_colours[segment + 1][1];
    segments[segment][2] = diagnostic_colours[segment + 1][2];
  }
  toggle++;
  memcpy(segments[segment] + 3, pixels, n*3);  // skip first pixel
  if (n > longest_sequence) {
    longest_sequence = n;
  }
}

static char button_name[5] = " yabx";
static int button_state[5] = {0, 0, 0, 0, 0};
static int last_button_read[5] = {0, 0, 0, 0, 0};
static int last_button_sequence_time = 0;
static char button_sequence[10] = "";
static int button_sequence_i = 0;
static int pressed_button = 0;
static int pressed_button_count = 0;
static int cancel_start_time = 0;

void update_buttons() {
  pressed_button = 0;
  pressed_button_count = 0;
  for (int b = 1; b <= 4; b++) {
    int state = tcl_read_button(b) ? 1 : 0;
    if (1 || state == last_button_read[b]) {  // debounce
      button_state[b] = state;
      if (state) {
        pressed_button = b;
      }
    }
    last_button_read[b] = state;
    pressed_button_count += button_state[b];
  }
}

const char* get_button_sequence() {
  return button_sequence;
}

void clear_button_sequence() {
  button_sequence[0] = 0;
  button_sequence_i = 0;
  last_button_sequence_time = 0;
}

int read_button(char button) {
  switch (button) {
    case 'Y':
    case 'y':
    case 1:
      return button_state[1];
    case 'A':
    case 'a':
    case 2:
      return button_state[2];
    case 'B':
    case 'b':
    case 3:
      return button_state[3];
    case 'X':
    case 'x':
    case 4:
      return button_state[4];
  }
  return 0;
}

int get_milliseconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

typedef struct {
  unsigned int version;
  unsigned int timestamp;
  int inst[3];
  int avg[3];
  unsigned int impact[3];
  unsigned int impact_time;
  unsigned int impact_hint;
  unsigned int range;
} acceldata;

void* shm_ptr = NULL;

void* connect_to_accelerometer(char* path) {
  int size = sizeof(acceldata);
  int shm_id;
  FILE* fp;
  key_t ipc_token;

  ipc_token = ftok(path, 'R');
  if (ipc_token == -1) {
    return NULL;
  }

  shm_id = shmget(ipc_token, size, 0644 | IPC_CREAT);
  if (shm_id == -1) {
    return NULL;
  }

  shm_ptr = shmat(shm_id, NULL, SHM_RDONLY);
  if (shm_ptr == (void*) -1) {
    return NULL;
  }

  return shm_ptr;
}

float accel_x_center = 0;
float accel_y_center = 0;
float accel_x = 0, accel_y = 0;
#define ACCEL_HISTORY (30*FPS)
int accel_x_history[ACCEL_HISTORY];
int accel_y_history[ACCEL_HISTORY];
int accel_x_sum = 0;
int accel_y_sum = 0;
int accel_sum_count = 0;
int accel_i = 0;

void read_accelerometer(char* filename, int frame) {
  static int idle_count = 0;
  static unsigned int last_timestamp = 0;

  if (!shm_ptr || idle_count > 10) {
    shm_ptr = connect_to_accelerometer(filename);
  }
  if (shm_ptr) {
    acceldata* accel = (acceldata*) shm_ptr;
    unsigned int timestamp = accel->timestamp;
    int x = accel->avg[0] - 2048;
    int y = accel->avg[1] - 2048;

    if (timestamp == last_timestamp) {
      idle_count++;
    }
    last_timestamp = timestamp;

    if (accel_sum_count == ACCEL_HISTORY) {
      accel_x_sum -= accel_x_history[accel_i];
      accel_y_sum -= accel_y_history[accel_i];
      accel_sum_count--;
    }
    accel_x_sum += x;
    accel_y_sum += y;
    accel_sum_count++;
    accel_x_history[accel_i] = x;
    accel_y_history[accel_i] = y;
    accel_i = (accel_i + 1) % ACCEL_HISTORY;

    if (frame < 5*FPS) {
      accel_x = 0;
      accel_y = 0;
    } else {
      accel_x = x - accel_x_sum/accel_sum_count;
      accel_y = y - accel_y_sum/accel_sum_count;
    }
  }
}

int accel_right() {
  return (accel_x > 15 || accel_x < -15) ? accel_x : 0;
}

int accel_forward() {
  return (accel_y > 15 || accel_y < -15) ? -accel_y : 0;
}

int main(int argc, char* argv[]) {
  int frame = 0;
  int start_time = get_milliseconds();
  int next_frame_time = start_time + 1000/FPS;
  int now;
  int s, i;
  int clock_delay = 0;
  int time_buffer[11], ti = 0, tf = 0;
  int last_button_count = 0;

  bzero(head, (1 + HEAD_PIXELS)*3);
  bzero(segments, NUM_SEGS*(1 + SEG_PIXELS)*3);
  tcl_init();
  tcl_set_clock_delay(clock_delay);
  while (1) {
    read_accelerometer("/tmp/.accel", frame);
    while (now < next_frame_time) {
      now = get_milliseconds();
    }
    longest_sequence = 0;
    next_frame(frame++);
    tcl_put_pixels_multi(strand_ptrs, 1 + NUM_SEGS, longest_sequence + 1);

    now = get_milliseconds();
    tf += (tf < 10);
    ti = (ti + 1) % 11;
    time_buffer[ti] = now;
    printf("frame %5d (%.1f fps)  %c%c%c%c  [%-10s]  ", frame,
           tf*1000.0/(now - time_buffer[(ti + 11 - tf) % 11]),
           read_button('a') ? 'a' : ' ',
           read_button('b') ? 'b' : ' ',
           read_button('x') ? 'x' : ' ',
           read_button('y') ? 'y' : ' ',
           button_sequence);
    if (accel_right() || accel_forward()) {
      printf("forward%+3d right%+3d\n", accel_forward(), accel_right());
    } else {
      printf("\r");
    }
    fflush(stdout);

    next_frame_time += 1000/FPS;

    update_buttons();
    if (last_button_count == 0 && pressed_button_count == 1) {
      if (button_name[pressed_button] == 'y') {
        cancel_start_time = now;
      } else {
        cancel_start_time = 0;
      }
      if (button_sequence_i < 10) {
        button_sequence[button_sequence_i++] = button_name[pressed_button];
        button_sequence[button_sequence_i] = 0;
        last_button_sequence_time = now;
      }
    }
    last_button_count = pressed_button_count;
    if (button_name[pressed_button] == 'y' ||
        now - last_button_sequence_time > 10000) {  // cancel
      clear_button_sequence();
    }
    if (cancel_start_time > 0 && button_name[pressed_button] == 'y' &&
        now - cancel_start_time > 2000) {
      break;
    }

    if (read_button('a') && read_button('x') && read_button('y')) {
      if (clock_delay < 100) {
        tcl_set_clock_delay(++clock_delay);
        printf("\ndelay %d\n", clock_delay);
        diagnostic_disco = 1;
      }
    }
    if (read_button('b') && read_button('x') && read_button('y')) {
      if (clock_delay > 0) {
        tcl_set_clock_delay(--clock_delay);
        printf("\ndelay %d\n", clock_delay);
        diagnostic_disco = 0;
      }
    }
  }
}
