#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

typedef struct {
  float x, y, z;
} vector;

typedef struct {
  float r, g, b;
} colour;

#define set_colour(c) ((temp_colour = c), glColor3fv(&(temp_colour.r)))
#define put_vertex(v) ((temp_vector = v), glVertex3fv(&(temp_vector.x)))

colour temp_colour;
vector temp_vector;

#define SEG_NK 50 // number of vertices along the length of a segment
#define SEG_NA 100 // number of vertices around the circumference of a segment
#define SEG_LENGTH 1.4 // m
#define SEG_SPACING 0.2 // m
#define NUM_SEGS 10 // number of segments in the whole serpent
#define TOTAL_LENGTH (NUM_SEGS*SEG_LENGTH + (NUM_SEGS - 1)*SEG_SPACING)
#define RADIUS 0.5  // m
#define SERPENT_T(seg, frac) \
    ((seg*(SEG_LENGTH + SEG_SPACING) + frac*SEG_LENGTH)/TOTAL_LENGTH)

#define FOV_DEGREES 30
int orbiting = 0, dollying = 0;
float start_angle, start_height, start_distance;
int start_x, start_y;
float orbit_angle = 0.0;
float tan_camera_height = 1.0;
float camera_distance = 20.0;
float camera_aspect = 1.0;

colour grid[(SEG_NK + 1)*SEG_NA];

// 0.0 <= t <= 1.0
vector compute_spine_point(float t) {
  vector result;
  result.x = (t - 0.5)*TOTAL_LENGTH;
  result.y = sin(t*4.0)*2;  // a gentle curve
  result.z = 0;
  return result;
}

float length(vector v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

vector normalize(vector v) {
  float len = length(v);
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

vector multiply(float f, vector v) {
  vector result;
  result.x = f*v.x;
  result.y = f*v.y;
  result.z = f*v.z;
  return result;
}

vector compute_cylinder_point(
    vector spine, vector up, vector right, int a, int na) {
  float angle = 2*M_PI*a/na;
  vector upward = multiply(cos(angle)*RADIUS, up);
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

// k is an index in the length direction, 0 <= k <= nk
// a is an index in the angular direction, 0 <= a < na
// grid is an array of colours indexed by [k][a], with (k+1)*a elements
void draw_segment(float tmin, float tmax, colour* grid, int nk, int na) {
  vector spine[nk + 1];
  vector up = {0, 0, 1};
  vector right[nk + 1];
  for (int k = 0; k <= nk; k++) {
    spine[k] = compute_spine_point(tmin + (tmax - tmin)*k/nk);
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
}

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  for (int s = 0; s < NUM_SEGS; s++) {
    draw_segment(SERPENT_T(s, 0), SERPENT_T(s, 1), grid, SEG_NK, SEG_NA);
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
  gluLookAt(-camera_distance, 0, tan_camera_height*camera_distance, // eye
            0, 0, 0, // target
            0, 0, 1); // up
  glRotatef(orbit_angle, 0, 0, 1);
  display();
}

void reshape(int width, int height) {
  glViewport(0, 0, width, height);
  camera_aspect = ((float) width)/((float) height);
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
    start_height = tan_camera_height;
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
    tan_camera_height = start_height + (y - start_y)*0.01;
    update_camera();
  }
  if (dollying) {
    camera_distance = start_distance + (y - start_y)*0.1;
    if (camera_distance < 1.0) {
      camera_distance = 1.0;
    }
    update_camera();
  }
}

void keyboard(unsigned char key, int x, int y) {
  if (key == '\x1b') {
    exit(0);
  }
}

void init(void) {
  /* Use depth buffering for hidden surface elimination. */
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);

  /* Set up some colours. */
  for (int k = 0; k <= SEG_NK; k++) {
    for (int a = 0; a < SEG_NA; a++) {
      grid[k*SEG_NA + a].r = k % 2;
      grid[k*SEG_NA + a].g = a % 2;
      grid[k*SEG_NA + a].b = 0.1;
    }
  }
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("Serpent");
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutKeyboardFunc(keyboard);
  init();
  glutMainLoop();
  return 0;
}
