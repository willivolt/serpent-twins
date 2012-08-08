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
#include <string.h>
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
#include "opc.h"

// Graphics types
typedef struct {
  double x, y, z;
} vector;

typedef struct {
  double r, g, b;
} colour;

colour temp_colour;
vector temp_vector;
#define set_colour(c) ((temp_colour = c), glColor4dv(&(temp_colour.r)))
#define put_vertex(v) ((temp_vector = v), glVertex3dv(&(temp_vector.x)))

// Camera parameters
#define FOV_DEGREES 30
int orbiting = 0, dollying = 0;
double start_angle, start_elevation, start_distance;
int start_x, start_y;
double orbit_angle = 178.0; // camera orbit angle, degrees
double camera_elevation = -10; // camera elevation angle, degrees
double camera_distance = 11.0; // metres
double camera_aspect = 1.0;

// Animation parameters
double next_frame_time;
int frame = 0, paused = 0;

double length(vector v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

// LED data
int num_pixels = 0;
pixel* pixels;
vector* points;

// OPC source
opc_source source = -1;


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

void draw_axes() {
  int i;
  vector o = {0, 0, 0};
  vector x = {1, 0, 0};
  vector y = {0, 1, 0};
  vector z = {0, 0, 1};
  colour r = {0.3, 0, 0};
  colour g = {0, 0.3, 0};
  colour b = {0, 0, 0.3};
  colour w = {0.3, 0.3, 0.3};
  glBegin(GL_POINTS);
  set_colour(w);
  put_vertex(o);
  for (i = 0; i < 100; i++) {
    set_colour(r);
    put_vertex(multiply(i*0.1, x));
    set_colour(g);
    put_vertex(multiply(i*0.1, y));
    set_colour(b);
    put_vertex(multiply(i*0.1, z));
  }
  glEnd();
}

void draw_octahedron(vector center, double radius) {
  vector x = {radius, 0, 0};
  vector y = {0, radius, 0};
  vector z = {0, 0, radius};

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

void draw_points(int n, pixel* pixels, vector* points) {
  int i;

  for (i = 0; i < n; i++) {
    colour c = {pixels[i].r/255.0, pixels[i].g/255.0, pixels[i].b/255.0};
    set_colour(c);
    draw_octahedron(points[i], 0.04);
  }
}

void display(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  draw_points(num_pixels, pixels, points);
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
  glutPostRedisplay();
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
  switch (key) {
    case '\x1b':
    case 'q':
      printf("\n");
      exit(0);
  }
}

double get_time() {
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec + 1e-6*now.tv_usec;
}

void pixel_handler(u8 recv_address, u16 recv_count, pixel* recv_pixels) {
  u16 count;

  if (recv_address == 1) {
    count = recv_count > num_pixels ? num_pixels : recv_count;
    memcpy(pixels, recv_pixels, count*sizeof(pixel));
    glutPostRedisplay();
  }
}

void idle(void) {
  opc_receive(source, pixel_handler, 4);  // we can afford to wait 4 ms
}

void init_pixel_coordinates(const char* command) {
  FILE* fp;
  int parsed, allocated;
  float x, y, z;

  fp = popen(command, "r");
  allocated = 16;
  pixels = malloc(allocated*sizeof(pixel));
  points = malloc(allocated*sizeof(vector));
  num_pixels = 0;
  while (1) {
    parsed = fscanf(fp, "%f %f %f\n", &x, &y, &z);
    if (parsed < 3) {
      break;
    }
    if (num_pixels >= allocated) {
      allocated *= 2;
      pixels = realloc(pixels, allocated*sizeof(pixel));
      points = realloc(points, allocated*sizeof(vector));
    }
    points[num_pixels].x = x;
    points[num_pixels].y = y;
    points[num_pixels].z = z;
    pixels[num_pixels].r = 255;
    pixels[num_pixels].g = 255;
    pixels[num_pixels].b = 255;
    num_pixels++;
  }
  fprintf(stderr, "Initialized %d pixels.\n", num_pixels);
  pclose(fp);
}

int main(int argc, char **argv) {
  u16 port;
  char* command;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("OPC");
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutIgnoreKeyRepeat(1);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);

  /* Use depth buffering for hidden surface elimination. */
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  /* Make glutSwapBuffers wait for vertical refresh to avoid frame tearing. */
#ifdef __APPLE__
  int swap_interval = 1;
  CGLContextObj context = CGLGetCurrentContext();
  CGLSetParameter(context, kCGLCPSwapInterval, &swap_interval);
#endif

  /* First argument should be the port number. */
  port = argc > 1 ? atoi(argv[1]) : 0;

  /* Second argument should be a command that emits pixel coordinates. */
  command = argc > 2 ? argv[2] : NULL;

  if (!port || !command) {
    fprintf(stderr, "Usage: %s <port> <command>\n", argv[0]);
    return 1;
  }

  source = opc_new_source(port);
  if (source < 0) {
    return 1;
  }
  init_pixel_coordinates(command);

  glutMainLoop();
  return 0;
}
