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
#include "tcp_pixels.h"
#include "midi.h"

#define JULUNGGUL 0  // white serpent
#define JORMUNGAND 1  // black serpent
static byte serpent_mode = JULUNGGUL;

static byte head[HEAD_PIXELS*3];
static byte segments[NUM_SEGS][(SEG_PIXELS + FIN_PIXELS + LID_PIXELS)*3];
static byte spine[NUM_ROWS*3];
static byte fins[NUM_SEGS*FIN_PIXELS*3];
static byte* strand_ptrs[1 + NUM_SEGS] = {
  head,
  segments[0], // barrel 1
  segments[1], // barrel 2
  segments[2], // barrel 3
  segments[3], // barrel 4
  segments[4], // barrel 5
  segments[5], // barrel 6
  segments[6], // barrel 7
  segments[7], // barrel 8
  segments[8], // barrel 9
  segments[9] // tail
};

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

// LED sequences on the black serpent
#define JORM_SPINE_FRONT 0
#define JORM_SPINE_BACK 32
#define JORM_SPINE_PIXELS 33
#define JORM_FINS_BACK 33
#define JORM_FINS_FRONT 42
#define JORM_FIN_PIXELS 10
#define JORM_RIGHT_FRONT 43
#define JORM_RIGHT_BACK 70
#define JORM_LEFT_FRONT 71
#define JORM_LEFT_BACK 98
#define JORM_SIDE_PIXELS 28
#define JORM_SEG_PIXELS 99

#define TAIL_FIN_START 100
#define TAIL_FIN_COUNT 12
#define TAIL_LANTERN_START 112
#define TAIL_LANTERN_COUNT 22
#define TAIL_PIXELS TAIL_LANTERN_START + TAIL_LANTERN_COUNT

byte jormungand_segment[JORM_SEG_PIXELS*3];

// On Jormungand, the spine pixels within a barrel alternate left and right.
// On some barrels, the frontmost spine pixel is on the left; on other barrels
// it's on the right.
byte jorm_spine_front_is_left[9] = {1, 0, 1, 0, 1, 0, 1, 0, 0};

void put_head_pixels(byte* pixels, int n) {
  memcpy(head, pixels, n*3);
}

void put_segment_pixels(int segment, byte* pixels, int n) {
  memcpy(segments[segment], pixels, n*3);
}

void put_spine_pixels(byte* pixels, int n) {
  memcpy(spine, pixels, n*3);
}

void put_fin_pixels(byte* pixels, int n) {
  int s, b, i;
  byte* dest;

  for (s = 0; n > 0 && s < NUM_SEGS; s++) {
    dest = segments[s] + (SEG_PIXELS + FIN_PIXELS)*3;
    if (s == 9) { // tail
      dest = segments[s] + (TAIL_FIN_START + TAIL_FIN_COUNT)*3;
    }
    for (i = 0; n > 0 && i < FIN_PIXELS; i++) {
      dest -= 3;
      dest[0] = *pixels++;
      dest[1] = *pixels++;
      dest[2] = *pixels++;
      n--;
      if (s == 9) {
        if (i == 0 || i == 1) {
          // skip the first two input pixels
          dest += 3;
          pixels -= 3;
        }
        if (i == 6) {
          // skip the output pixels between the tail fins
          dest -= 15;
        }
      }
    }
  }
}

void get_column(pixel* segment, int c, pixel* column) {
  int r;
  for (r = 0; r < SEG_ROWS; r++) {
    column[r] = segment[NUM_COLUMNS*r + (r % 2 ? NUM_COLUMNS - 1 - c : c)];
  }
}

pixel rgb_interpolate(pixel a, pixel b, float f) {
  float v;
  pixel c;
  v = a.r * (1.0 - f) + b.r * f;
  c.r = (v < 0) ? 0 : (v > 255) ? 255 : v;
  v = a.g * (1.0 - f) + b.g * f;
  c.g = (v < 0) ? 0 : (v > 255) ? 255 : v;
  v = a.b * (1.0 - f) + b.b * f;
  c.b = (v < 0) ? 0 : (v > 255) ? 255 : v;
  return c;
}

void remap_to_jormungand(int s, byte* segment, byte* jorm_segment) {
  int i, k;
  byte left;
  pixel* column;
  pixel left_column[NUM_ROWS], right_column[NUM_ROWS];
  pixel* jorm_pixels = (pixel*) jorm_segment;
  pixel* dest;
  float t;

  get_column((pixel*) segment, 20, left_column);
  get_column((pixel*) segment, 4, right_column);

  bzero(jorm_segment, JORM_SEG_PIXELS*3);
  dest = jorm_pixels + JORM_SPINE_FRONT;
  for (i = 0, left = jorm_spine_front_is_left[s];
       i < JORM_SPINE_PIXELS; i++, left = !left) {
    t = (SEG_ROWS - 1) * ((float) i)/(JORM_SPINE_PIXELS - 1);
    k = (int) t;
    column = (left ? left_column : right_column);
    *dest = rgb_interpolate(column[k], column[k + 1], t - k);
    dest++;
  }

  get_column((pixel*) segment, 15, left_column);
  get_column((pixel*) segment, 9, right_column);

  dest = jorm_pixels + JORM_RIGHT_FRONT;
  for (i = 0; i < JORM_SIDE_PIXELS; i++) {
    t = (SEG_ROWS - 1) * (((float) i)/(JORM_SIDE_PIXELS - 1));
    k = (int) t;
    jorm_pixels[JORM_RIGHT_FRONT + i] =
        rgb_interpolate(right_column[k], right_column[k + 1], t - k);
    jorm_pixels[JORM_LEFT_FRONT + i] =
        rgb_interpolate(left_column[k], left_column[k + 1], t - k);
  }

  column = ((pixel*) segment) + SEG_PIXELS;
  for (i = 0; i < JORM_FIN_PIXELS; i++) {
    t = (FIN_PIXELS - 1) * (((float) i)/(JORM_FIN_PIXELS - 1));
    k = (int) t;
    jorm_pixels[JORM_FINS_BACK + i] =
        rgb_interpolate(column[k], column[k + 1], t - k);
  }
}

void average_rgb(byte* dest, byte* pixels, int n) {
  int i;
  int red = 0, green = 0, blue = 0;
  byte* p = pixels;
  for (i = 0; i < n; i++) {
    red += *(p++);
    green += *(p++);
    blue += *(p++);
  }
  dest[0] = (byte) (red/n);
  dest[1] = (byte) (green/n);
  dest[2] = (byte) (blue/n);
}

void set_lid_pixels() {
  int s;
  byte* row;
  byte* lid;
  byte temp[6];
  // The frontmost circle of barrel pixels starts at bottom and proceeds toward
  // the serpent's right (clockwise if you are facing the barrel looking toward
  // the tail).  The first 8 lid pixels start at the top and also proceed
  // clockwise; the last lid pixel is in the center.
  for (s = 0; s < NUM_SEGS; s++) {
    row = segments[s];
    lid = segments[s] + (SEG_PIXELS + FIN_PIXELS)*3;
    average_rgb(lid + 0*3, row + 11*3, 3);
    average_rgb(lid + 1*3, row + 14*3, 3);
    average_rgb(lid + 2*3, row + 17*3, 3);
    average_rgb(lid + 3*3, row + 20*3, 3);
    average_rgb(temp, row + 23*3, 2);
    average_rgb(temp + 3, row + 0*3, 2);
    average_rgb(lid + 4*3, temp, 2);
    average_rgb(lid + 5*3, row + 2*3, 3);
    average_rgb(lid + 6*3, row + 5*3, 3);
    average_rgb(lid + 7*3, row + 8*3, 3);
    average_rgb(lid + 8*3, row, 25);
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
  static float fcount = 0;
  int frame = 0;
  int start_time = get_milliseconds();
  int next_frame_time = start_time + 1000/FPS;
  int now;
  int s, i, j;
  FILE* fp;
  int clock_delay = 0;
  int time_buffer[11], ti = 0, tf = 0;
  int last_button_count = 0;

  serpent_mode = getenv("BLACK_SERPENT") ? JORMUNGAND : JULUNGGUL;

  bzero(head, HEAD_PIXELS*3);
  bzero(segments, NUM_SEGS*(SEG_PIXELS + FIN_PIXELS)*3);

  // Assign addresses to all the barrels in the order they're connected.
  tcp_init();

  midi_init();
  midi_set_control(6, 10);

  while (1) {
    midi_poll();

    while (now < next_frame_time) {
      now = get_milliseconds();
    }

    next_frame(frame++);

    if (midi_get_control(6) > 0 && midi_get_control(6) < 16) {
      // White fin chaser light
      int n = NUM_SEGS*FIN_PIXELS;
      float speed = (midi_get_control(6) - 8)/3.0;
      fcount += speed;
      if (fcount < -n/2) { fcount += n; }
      if (fcount > n/2) { fcount -= n; }
      for (i = 0; i < n; i++) {
        j = i;
        float dist = (fcount - j) / (fabs(speed) + 1);
        int fin_bright = (int) (600.0/(1 + dist*dist));
        dist = (fcount - (j - n)) / speed;
        fin_bright += (int) (600.0/(1 + dist*dist));
        dist = (fcount - (j - n/2)) / speed;
        fin_bright += (int) (600.0/(1 + dist*dist));
        dist = (fcount - (j + n/2)) / speed;
        fin_bright += (int) (600.0/(1 + dist*dist));
        dist = (fcount - (j + n)) / speed;
        fin_bright += (int) (600.0/(1 + dist*dist));
        if (fin_bright > 255) fin_bright = 255;
        fins[i*3 + 0] = fin_bright;
        fins[i*3 + 1] = fin_bright;
        fins[i*3 + 2] = fin_bright;
      }
    } else {
      float fin_level = (midi_get_control(6) - 31)/96.0;
      if (fin_level < 0) {
        fin_level = 0;
      }
      for (i = 0; i < NUM_SEGS*FIN_PIXELS; i++) {
        float t = (NUM_ROWS - 1) * ((float) i)/(NUM_SEGS*FIN_PIXELS - 1);
        int k = (int) t;
        pixel f = rgb_interpolate(
            ((pixel*)spine)[k], ((pixel*)spine)[k + 1], t - k);
        fins[i*3 + 0] = (int) f.r * fin_level;
        fins[i*3 + 1] = (int) f.g * fin_level;
        fins[i*3 + 2] = (int) f.b * fin_level;
      }
    }

    put_fin_pixels(fins, NUM_SEGS*FIN_PIXELS);

    switch (serpent_mode) {
      case JULUNGGUL:
        set_lid_pixels();
        tcp_put_pixels(1, head, HEAD_PIXELS);
        for (s = 0; s < NUM_SEGS; s++) {
          tcp_put_pixels(2 + s, segments[s],
                         SEG_PIXELS + FIN_PIXELS + LID_PIXELS);
        }
        break;
      case JORMUNGAND:
        tcp_put_pixels(1, head, HEAD_PIXELS);
        for (s = 0; s < NUM_SEGS - 1; s++) {
          remap_to_jormungand(s, segments[s], jormungand_segment);
          tcp_put_pixels(2 + s, jormungand_segment, JORM_SEG_PIXELS);
        }
        tcp_put_pixels(11, segments[9], TAIL_PIXELS);
        break;
    }

    now = get_milliseconds();
    tf += (tf < 10);
    ti = (ti + 1) % 11;
    time_buffer[ti] = now;
    printf("frame %5d (%4.1f fps)  [%c%c%c%c%c%c%c%c] %02x %02x %02x %02x %02x %02x %02x %02x  \r", frame,
           tf*1000.0/(now - time_buffer[(ti + 11 - tf) % 11]),
           midi_get_note(1) > 0 ? '1' : ' ',
           midi_get_note(2) > 0 ? '2' : ' ',
           midi_get_note(3) > 0 ? '3' : ' ',
           midi_get_note(4) > 0 ? '4' : ' ',
           midi_get_note(5) > 0 ? '5' : ' ',
           midi_get_note(6) > 0 ? '6' : ' ',
           midi_get_note(7) > 0 ? '7' : ' ',
           midi_get_note(8) > 0 ? '8' : ' ',
           midi_get_control(1),
           midi_get_control(2),
           midi_get_control(3),
           midi_get_control(4),
           midi_get_control(5),
           midi_get_control(6),
           midi_get_control(7),
           midi_get_control(8));
    fflush(stdout);

    next_frame_time += 1000/FPS;

    // update_buttons();
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
      }
    }
    if (read_button('b') && read_button('x') && read_button('y')) {
      if (clock_delay > 0) {
        tcl_set_clock_delay(--clock_delay);
        printf("\ndelay %d\n", clock_delay);
      }
    }
  }

  fprintf(stderr, "Loop terminated.\n");
}
