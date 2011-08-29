// Copyright 2011 Google Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "serpent.h"

typedef struct {
  double x, y, z;
} vector;

typedef struct {
  double r, g, b;
} colour;

typedef struct {
  unsigned char r, g, b;
} led_colour;

#define set_colour(c) ((temp_colour = c), glColor3dv(&(temp_colour.r)))
#define put_vertex(v) ((temp_vector = v), glVertex3dv(&(temp_vector.x)))

colour temp_colour;
vector temp_vector;

// Serpent physical size
#define SEG_LENGTH 1.4 // metres
#define SEG_SPACING 0.2 // metres
#define NUM_SEGS 10 // number of segments in the whole serpent
#define TOTAL_LENGTH (NUM_SEGS*SEG_LENGTH + (NUM_SEGS - 1)*SEG_SPACING)
#define RADIUS 0.5 // metres

// Camera parameters
#define FOV_DEGREES 30
int orbiting = 0, dollying = 0;
double start_angle, start_elevation, start_distance;
int start_x, start_y;
double orbit_angle = 174.0; // camera orbit angle, degrees
double camera_elevation = 30; // camera elevation angle, degrees
double camera_distance = 11.0; // metres
double camera_aspect = 1.0;

// LED colours
#define LED_NK 12 // number of LEDs along the length of a segment
#define LED_NA 25 // number of LEDs around the circumference of a segment
led_colour leds[NUM_SEGS][LED_NK][LED_NA];
led_colour head_leds[HEAD_PIXELS];
colour BASE_COLOUR = {0.05, 0.05, 0.05};
colour curves[256];

// Serpent rendering resolution ("unit" = "distance between vertices")
#define PAD_UNITS 2 // number of units from edge to first ring of LEDs
#define LED_NK_UNITS 2 // number of units between adjacent rings of LEDs
#define LED_NA_UNITS 2 // number of units between adjacent LEDs on a ring
#define SEG_NK (PAD_UNITS*2 + LED_NK_UNITS*(LED_NK - 1)) // units along length
#define SEG_NA (LED_NA_UNITS*LED_NA) // units around segment circumference
colour render_grids[NUM_SEGS][(SEG_NK + 1)*SEG_NA];

// Blur convolution matrix
#define BLUR_Z 1.5 // depth of LED under surface, in render units
#define BLUR_RADIUS 8
#define BLUR_WIDTH (BLUR_RADIUS*2 + 1)
#define BLUR_BRIGHTNESS_SCALE 0.15
double blur[BLUR_WIDTH][BLUR_WIDTH];

// Animation parameters
double next_frame_time;
int frame = 0, paused = 0;


vector compute_spine_point(int segment, double fraction) {
  double t = segment*(SEG_LENGTH + SEG_SPACING) + fraction*SEG_LENGTH;
  vector result;
  result.x = TOTAL_LENGTH/2 - t;
  result.y = sin(4*t/TOTAL_LENGTH)*2;  // a gentle curve
  result.z = 0;
  return result;
}

double length(vector v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

vector normalize(vector v) {
  double len = length(v);
  vector result;
  result.x = v.x/len;
  result.y = v.y/len;
  result.z = v.z/len;
  return result;
}

vector cross(vector v, vector w) {
  vector result;
  result.x = v.y*w.z - v.z*w.y;
  result.y = v.z*w.x - v.x*w.z;
  result.z = v.x*w.y - v.y*w.x;
  return result;
}

vector add(vector v, vector w) {
  vector result;
  result.x = v.x + w.x;
  result.y = v.y + w.y;
  result.z = v.z + w.z;
  return result;
}

vector subtract(vector v, vector w) {
  vector result;
  result.x = v.x - w.x;
  result.y = v.y - w.y;
  result.z = v.z - w.z;
  return result;
}

vector multiply(double f, vector v) {
  vector result;
  result.x = f*v.x;
  result.y = f*v.y;
  result.z = f*v.z;
  return result;
}

vector compute_cylinder_point(
    vector spine, vector up, vector right, int a, int na) {
  double angle = 2*M_PI*a/na;
  vector upward = multiply(-cos(angle)*RADIUS, up);  // assume seam on bottom
  vector rightward = multiply(sin(angle)*RADIUS, right);
  return add(add(spine, upward), rightward);
}

void draw_axes() {
  vector o = {0, 0, 0};
  vector x = {1, 0, 0};
  vector y = {0, 1, 0};
  vector z = {0, 0, 1};
  colour r = {1, 0, 0};
  colour g = {0, 1, 0};
  colour b = {0, 0, 1};
  colour w = {1, 1, 1};
  glBegin(GL_LINES);
  set_colour(w);
  put_vertex(o);
  put_vertex(x);
  put_vertex(o);
  put_vertex(y);
  put_vertex(o);
  put_vertex(z);
  set_colour(r);
  put_vertex(o);
  put_vertex(multiply(10, x));
  set_colour(g);
  put_vertex(o);
  put_vertex(multiply(10, y));
  set_colour(b);
  put_vertex(o);
  put_vertex(multiply(10, z));
  glEnd();
}

void draw_disc(vector center, vector up, vector right, int na) {
  glBegin(GL_TRIANGLE_FAN);
  put_vertex(center);
  for (int ai = 0; ai <= na; ai++) {
    int a = ai % na;
    put_vertex(compute_cylinder_point(center, up, right, a, na));
  }
  glEnd();
}

// k is an index in the length direction, 0 <= k <= nk
// a is an index in the angular direction, 0 <= a < na
// grid is an array of colours indexed by [k][a], with (k+1)*a elements
void draw_segment(int segment, colour* grid, int nk, int na) {
  vector spine[nk + 1];
  vector up = {0, 0, 1};
  vector right[nk + 1];
  for (int k = 0; k <= nk; k++) {
    spine[k] = compute_spine_point(segment, ((double) k)/nk);
  }
  for (int k = 0; k <= nk; k++) {
    vector previous = spine[k > 0 ? k - 1 : 0];
    vector next = spine[k < nk ? k + 1 : nk];
    vector forward = normalize(subtract(next, previous));
    right[k] = cross(forward, up);
  }
  for (int k = 0; k < nk; k++) {
    int k1 = k + 1;
    glBegin(GL_QUAD_STRIP);
    for (int ai = 0; ai <= na; ai++) {
      int a = ai % na;
      set_colour(grid[k*na + a]);
      put_vertex(compute_cylinder_point(spine[k], up, right[k], a, na));
      set_colour(grid[k1*na + a]);
      put_vertex(compute_cylinder_point(spine[k1], up, right[k1], a, na));
    }
    glEnd();
  }
  set_colour(BASE_COLOUR);
  draw_disc(spine[0], up, right[0], na);
  draw_disc(spine[nk], up, right[nk], na);
}

void draw_octahedron(vector center, double size) {
  vector x = {size, 0, 0};
  vector y = {0, size, 0};
  vector z = {0, 0, size};
  
  glBegin(GL_TRIANGLE_FAN);
  put_vertex(add(center, z));
  put_vertex(add(center, x));
  put_vertex(add(center, y));
  put_vertex(subtract(center, x));
  put_vertex(subtract(center, y));
  put_vertex(add(center, x));
  glEnd();
  glBegin(GL_TRIANGLE_FAN);
  put_vertex(subtract(center, z));
  put_vertex(add(center, x));
  put_vertex(add(center, y));
  put_vertex(subtract(center, x));
  put_vertex(subtract(center, y));
  put_vertex(add(center, x));
  glEnd();
}

#define draw_oct(f, r, u) draw_octahedron( \
    add(base, add(multiply(f, forward), \
    add(multiply(u, up), multiply(r, right)))), 0.04)

struct {
  int count;
  double forward_scale;
  double right;
  double up;
} head_segments[] = {
  {182, 0.02, -0.3, 0.4}, // left wing
  {22, 0.08, -0.3, 0.2}, // left outer eye
  {13, 0.08, -0.3, 0}, // left inner eye
  {12 + 6, 0.08, 0, -0.4}, // mouth
  {22, 0.08, 0.3, 0.2}, // right outer eye
  {13, 0.08, 0.3, 0}, // right inner eye
  {182, 0.02, 0.3, 0.4}, // right wing
  {0, 0, 0} // sentinel
};

void draw_head() {
  vector base = compute_spine_point(0, 0);
  vector previous = compute_spine_point(0, 0.1);
  vector forward = normalize(subtract(base, previous));
  vector up = {0, 0, 1};
  vector right = cross(forward, up);
  int p = 0;
  
  for (int s = 0; head_segments[s].count; s++) {
    for (int i = 0; i < head_segments[s].count; i++) {
      colour c = {head_leds[p].r/255.0,
                  head_leds[p].g/255.0,
                  head_leds[p].b/255.0};
      set_colour(c);
      draw_oct(i*head_segments[s].forward_scale,
               head_segments[s].right, head_segments[s].up);
      p++;
    }
  }
}

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw_head();
  for (int s = 0; s < NUM_SEGS; s++) {
    draw_segment(s, render_grids[s], SEG_NK, SEG_NA);
  }
  draw_axes();
  glutSwapBuffers();
}

void update_camera() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(FOV_DEGREES, camera_aspect, 0.1, 1e3); // fov, aspect, zrange
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  double camera_y = -cos(camera_elevation*M_PI/180)*camera_distance;
  double camera_z = sin(camera_elevation*M_PI/180)*camera_distance;
  gluLookAt(0, camera_y, camera_z, /* target */ 0, 0, 0, /* up */ 0, 0, 1);
  glRotatef(orbit_angle, 0, 0, 1);
  display();
}

void reshape(int width, int height) {
  glViewport(0, 0, width, height);
  camera_aspect = ((double) width)/((double) height);
  update_camera();
}

void mouse(int button, int state, int x, int y) {
  if (state == GLUT_DOWN && glutGetModifiers() & GLUT_ACTIVE_SHIFT) {
    dollying = 1;
    start_distance = camera_distance;
    start_x = x;
    start_y = y;
  } else if (state == GLUT_DOWN) {
    orbiting = 1;
    start_angle = orbit_angle;
    start_elevation = camera_elevation;
    start_x = x;
    start_y = y;
  } else {
    orbiting = 0;
    dollying = 0;
  }
}

void motion(int x, int y) {
  if (orbiting) {
    orbit_angle = start_angle + (x - start_x)*1.0;
    double elevation = start_elevation + (y - start_y)*1.0;
    camera_elevation = elevation < -89 ? -89 : elevation > 89 ? 89 : elevation;
    update_camera();
  }
  if (dollying) {
    double distance = start_distance + (y - start_y)*0.1;
    camera_distance = distance < 1.0 ? 1.0 : distance;
    update_camera();
  }
}

static int button_bits = 0;
static char button_name[5] = " yabx";
static int button_state[5] = {0, 0, 0, 0, 0};
static int last_button_read[5] = {0, 0, 0, 0, 0};
static int last_button_sequence_time = 0;
static char button_sequence[10] = "";
static int button_sequence_i = 0;
static int pressed_button = 0;
static int pressed_button_count = 0;

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case '\x1b':
    case 'q':
      printf("\n");
      exit(0);
    case ' ':
      paused = !paused;
      break;
    case 'y':
    case '1':
      button_bits |= 1;
      break;
    case 'a':
    case '2':
      button_bits |= 2;
      break;
    case 'b':
    case '3':
      button_bits |= 4;
      break;
    case 'x':
    case '4':
      button_bits |= 8;
      break;
  }
}

void keyboard_up(unsigned char key, int x, int y) {
  switch (key) {
    case 'y':
    case '1':
      button_bits &= ~1;
      break;
    case 'a':
    case '2':
      button_bits &= ~2;
      break;
    case 'b':
    case '3':
      button_bits &= ~4;
      break;
    case 'x':
    case '4':
      button_bits &= ~8;
      break;
  }
}

void update_buttons() {
  pressed_button = 0;
  pressed_button_count = 0;
  int mask = 1;
  for (int b = 1; b <= 4; b++) {
    int state = (button_bits & mask) ? 1 : 0;
    mask <<= 1;
    if (state == last_button_read[b]) {  // debounce
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

static int accel_seq[8][10] = {
  {30, 33, 29, 16, 10, 0, 44, -32, -10, 0},
  {21, 23, 39, 0, 21, 28, -29, -27, -17, -14},
  {29, 0, -16, -10, 0, 0, 0, 0, 0, 0},
  {12, 18, 17, 10, 0, 0, -12, -27, 0, 0},
  {18, 43, 35, -19, -37, -16, -32, -19, -11, 0},
  {26, 43, 56, 33, 0, -35, -69, -46, -28, -10},
  {13, 26, 38, 40, 36, 22, 0, -25, -57, -19},
  {37, 48, 49, 26, 13, 0, -19, -36, -32, -18}
};
static int accel_f_start = -1, accel_f_dir, accel_f_seq;
static int accel_r_start = -1, accel_r_dir, accel_r_seq;

void special(int key, int x, int y) {
  switch (key) {
    case GLUT_KEY_UP:
      accel_f_start = frame;
      accel_f_dir = 1;
      accel_f_seq = random() % 8;
      break;
    case GLUT_KEY_DOWN:
      accel_f_start = frame;
      accel_f_dir = -1;
      accel_f_seq = random() % 8;
      break;
    case GLUT_KEY_LEFT:
      accel_r_start = frame;
      accel_r_dir = -1;
      accel_r_seq = random() % 8;
      break;
    case GLUT_KEY_RIGHT:
      accel_r_start = frame;
      accel_r_dir = 1;
      accel_r_seq = random() % 8;
      break;
  }
}

void special_up(int key, int x, int y) {
  switch (key) {
    case GLUT_KEY_UP:
    case GLUT_KEY_DOWN:
      accel_f_start = -1;
      break;
    case GLUT_KEY_LEFT:
    case GLUT_KEY_RIGHT:
      accel_r_start = -1;
      break;
  }
}

int read_button(char b) {
  switch (b) {
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

int accel_right() {
  if (accel_r_start >= 0) {
    int offset = frame - accel_r_start;
    if (offset >= 0 && offset < 10) {
      return accel_seq[accel_r_seq][offset]*accel_r_dir;
    }
    accel_r_start = -1;
  }
  return 0;
}

int accel_forward() {
  if (accel_f_start >= 0) {
    int offset = frame - accel_f_start;
    if (offset >= 0 && offset < 10) {
      return accel_seq[accel_f_seq][offset]*accel_f_dir;
    }
    accel_f_start = -1;
  }
  return 0;
}

void update_render_grid() {
  int limit = (SEG_NK + 1)*SEG_NA;
  for (int s = 0; s < NUM_SEGS; s++) {
    for (int k = 0; k <= SEG_NK; k++) {
      for (int a = 0; a < SEG_NA; a++) {
        render_grids[s][k*SEG_NA + a] = BASE_COLOUR;
      }
    }
    for (int lk = 0; lk < LED_NK; lk++) {
      for (int la = 0; la < LED_NA; la++) {
        int k = PAD_UNITS + lk*LED_NK_UNITS;
        int a = la*LED_NA_UNITS + LED_NA_UNITS/2;
        double r = curves[leds[s][lk][la].r].r;
        double g = curves[leds[s][lk][la].g].g;
        double b = curves[leds[s][lk][la].b].b;
        for (int dx = -BLUR_RADIUS; dx <= BLUR_RADIUS; dx++) {
          for (int dy = -BLUR_RADIUS; dy <= BLUR_RADIUS; dy++) {
            if (k + dx >= 0 && k + dx <= SEG_NK) {
              int index = (k + dx)*SEG_NA + ((a + dy + SEG_NA) % SEG_NA);
              double brightness = blur[dx + BLUR_RADIUS][dy + BLUR_RADIUS];
              render_grids[s][index].r += r*brightness;
              render_grids[s][index].g += g*brightness;
              render_grids[s][index].b += b*brightness;
            }
          }
        }
      }
    }
  }
}

double get_time() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec + 1e-6*now.tv_usec;
}


double time_buffer[11];
int ti = 0, tf = 0;
int last_button_count = 0;

void idle(void) {
  if (!paused) {
    double now = get_time();
    if (now >= next_frame_time) {
      next_frame(frame++);
      update_render_grid();
      display();

      now = get_time();
      tf += (tf < 10);
      ti = (ti + 1) % 11;
      time_buffer[ti] = now;
      printf("frame %5d (%.1f fps)  %c%c%c%c  [%-10s]  ", frame,
             tf/(now - time_buffer[(ti + 11 - tf) % 11]),
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

      if (now > next_frame_time + 1.0) {
        next_frame_time = now;
        time_buffer[ti] = now;
        tf = 0;
      }
      next_frame_time += 1.0/FPS;

      update_buttons();
      if (last_button_count == 0 && pressed_button_count == 1) {
        if (button_sequence_i < 10) {
          button_sequence[button_sequence_i++] = button_name[pressed_button];
          button_sequence[button_sequence_i] = 0;
          last_button_sequence_time = now;
        }
      }
      last_button_count = pressed_button_count;
      if (button_name[pressed_button] == 'y' ||
          now - last_button_sequence_time > 10) {  // cancel
        button_sequence_i = 0;
        button_sequence[0] = 0;
      }
    }
  }
}

void init(void) {
  /* Use depth buffering for hidden surface elimination. */
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  
  /* Make glutSwapBuffers wait for vertical refresh to avoid frame tearing. */
#ifdef __APPLE__
  int swap_interval = 1;
  CGLContextObj context = CGLGetCurrentContext();
  CGLSetParameter(context, kCGLCPSwapInterval, &swap_interval);
#endif

  /* Set up the blur convolution matrix. */
  for (int x = 0; x < BLUR_WIDTH; x++) {
    for (int y = 0; y < BLUR_WIDTH; y++) {
      int dx = x - BLUR_RADIUS;
      int dy = y - BLUR_RADIUS;
      double distance_squared = dx*dx + dy*dy + BLUR_Z*BLUR_Z;
      blur[x][y] = BLUR_Z*BLUR_Z/distance_squared*BLUR_BRIGHTNESS_SCALE;
    }
  }

  /* Set up the colour transfer curves. */
  for (int i = 0; i < 256; i++) {
    curves[i].r = log(i + 1)/log(256);
    curves[i].g = log(i + 1)/log(256);
    curves[i].b = log(i + 1)/log(256);
  }

  /* Set up the LED grids. */
  for (int s = 0; s < NUM_SEGS; s++) {
    for (int k = 0; k < LED_NK; k++) {
      for (int a = 0; a < LED_NA; a++) {
        leds[s][k][a].r = 0;
        leds[s][k][a].g = 0;
        leds[s][k][a].b = 0;
      }
    }
  }

  time_buffer[0] = get_time();
  next_frame_time = time_buffer[0] + 1.0/FPS;
}

/* The main public interface.  Implement next_frame() and call this. */
void put_head_pixels(unsigned char* pixels, int n) {
  for (int i = 0; i < n; i++) {
    head_leds[i].r = *(pixels++);
    head_leds[i].g = *(pixels++);
    head_leds[i].b = *(pixels++);
  }
}

void put_segment_pixels(int segment, unsigned char* pixels, int n) {
  for (int i = 0; i < n; i++) {
    int k = i/LED_NA;
    int a = (k % 2) ? (i % LED_NA) : (LED_NA - 1 - i % LED_NA);
    leds[segment][k][a].r = *(pixels++);
    leds[segment][k][a].g = *(pixels++);
    leds[segment][k][a].b = *(pixels++);
  }
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("Serpent");
  glutReshapeWindow(1000, 360);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutIgnoreKeyRepeat(1);
  glutKeyboardFunc(keyboard);
  glutKeyboardUpFunc(keyboard_up);
  glutSpecialFunc(special);
  glutSpecialUpFunc(special_up);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
  return 0;
}
