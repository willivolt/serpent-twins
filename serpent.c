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
double orbit_angle = 200.0; // camera orbit angle, degrees
double camera_elevation = 30; // camera elevation angle, degrees
double camera_distance = 25.0; // metres
double camera_aspect = 1.0;

// LED colours
#define LED_NK 12 // number of LEDs along the length of a segment
#define LED_NA 25 // number of LEDs around the circumference of a segment
led_colour leds[NUM_SEGS][LED_NK][LED_NA];
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
#define BLUR_RADIUS 10
#define BLUR_WIDTH (BLUR_RADIUS*2 + 1)
#define BLUR_BRIGHTNESS_SCALE 0.15
double blur[BLUR_WIDTH][BLUR_WIDTH];

// Animation parameters
double start_time;
double next_frame_time;
int frame = 0, paused = 0;

#define FPS 20;


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

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void keyboard(unsigned char key, int x, int y) {
  if (key == '\x1b' || key == 'q') {
    printf("\n");
    exit(0);
  }
  if (key == ' ') {
    paused = !paused;
  }
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

void idle(void) {
  if (!paused) {
    double now = get_time();
    if (now >= next_frame_time) {
      next_frame(frame);
      update_render_grid();
      display();
      next_frame_time += 1.0/FPS;
      frame++;
      printf("frame %d (%.1f fps)\r", frame, frame/(now - start_time));
      fflush(stdout);
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
    curves[i].r = i ? 0.2 + 0.8*(i/255.0) : 0;
    curves[i].g = i ? 0.2 + 0.8*(i/255.0) : 0;
    curves[i].b = i ? 0.2 + 0.8*(i/255.0) : 0;
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

  next_frame_time = start_time = get_time();
}

/* The main public interface.  Implement next_frame() and call this. */
void put_pixels(int segment, unsigned char* pixels, int n) {
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
  glutReshapeWindow(1024, 768);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
  return 0;
}
