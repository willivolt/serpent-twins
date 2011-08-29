// "squares", by Brian Schantz

// Copyright 2011 Brian Schantz and Google Inc.
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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "serpent.h"

#define SQUARES_NUM_SPRITES 90
#define SQUARES_MAX_W 16      // max width (around circ.) of a squares_sprite
#define SQUARES_MAX_H 24      // max height (length of serpent) of a squares_sprite
#define SQUARES_MAX_Z 100     // max # of Z-levels (could improve this by making sure no two squares_sprites are on the same Z-level)
#define SQUARES_MAX_DX 2.0      // max X-axis movement in pix/frame
#define SQUARES_MAX_DY 7.5      // max Y-axis movement in pix/frame
#define SQUARES_MAX_BG_BLINK 160  // max # of frames between flashes

typedef struct {
  int w,h,z;
  float x,y;
  int r,g,b;
  float dx, dy;
} squares_sprite;

typedef struct {
  int r,g,b;
} squares_color;

void squares_init_sprites(void);
squares_sprite* squares_top_sprite(int x, int y);
void squares_move_sprites(void);

unsigned char pixels[NUM_PIXELS*3];
squares_sprite squares_sprites[SQUARES_NUM_SPRITES];
squares_color squares_bg;
int squares_blinktimer;

void next_frame(int frame) {
  if (frame == 0) {
    squares_init_sprites();
  }
  
  switch(squares_blinktimer) {
    case 5:
      squares_bg.r = squares_bg.g = squares_bg.b = 128;
      break;
    case 4:
      squares_bg.r = squares_bg.g = squares_bg.b = 255;
      break;
    case 3:
      squares_bg.r = squares_bg.g = squares_bg.b = 192;
      break;
    case 2:
      squares_bg.r = squares_bg.g = squares_bg.b = 128;
      break;
    case 1:
      squares_bg.r = squares_bg.g = squares_bg.b = 64;
      break;
    case 0:
      squares_blinktimer = rand()%SQUARES_MAX_BG_BLINK;
      // fall through intentionally
    default:
      squares_bg.r = squares_bg.g = squares_bg.b = 0;
      break;
  }
  squares_blinktimer--;
      
    for (int i = 0; i < NUM_PIXELS; i++) {
      int x = (i % NUM_COLUMNS);  // theta.  0 to 24
        int y = (i / NUM_COLUMNS);  // along cylinder.  0 to NUM_SEGS*SEG_ROWS (120)
        // reverse every other row
        if (y % 2 == 1) {
            x = (NUM_COLUMNS-1)-x;
        }

    squares_sprite* s = squares_top_sprite(x,y);
    if ( NULL == s ) {
      pixels[i*3] = squares_bg.r;
      pixels[i*3+1] = squares_bg.g;
      pixels[i*3+2] = squares_bg.b;
    } else {
      pixels[i*3] = s->r;
      pixels[i*3+1] = s->g;
      pixels[i*3+2] = s->b;
    }
  }

  for (int s = 0; s < NUM_SEGS; s++) {
    put_segment_pixels(s, pixels + 3*SEG_PIXELS*s, SEG_PIXELS);
  }
  squares_move_sprites();
}

void squares_init_sprites() {
  srand( time(NULL) );
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprites[i].w = rand()%SQUARES_MAX_W;
    squares_sprites[i].h = rand()%SQUARES_MAX_H;
    squares_sprites[i].z = rand()%SQUARES_MAX_Z;
    squares_sprites[i].r = rand()%256;
    squares_sprites[i].g = rand()%256;
    squares_sprites[i].b = rand()%256;
    float r = (float)rand()/(float)RAND_MAX;
    squares_sprites[i].x = r * (SQUARES_MAX_W*2+NUM_COLUMNS);
    r = (float)rand()/(float)RAND_MAX;
    squares_sprites[i].y = r * (SQUARES_MAX_H*2+NUM_ROWS);
    r = (float)(rand()-rand())/(float)RAND_MAX;
    squares_sprites[i].dx = r * SQUARES_MAX_DX;
    r = (float)(rand()-rand())/(float)RAND_MAX;
    squares_sprites[i].dy = r * SQUARES_MAX_DX;
  }
}

squares_sprite* squares_top_sprite(int x, int y) {
  squares_sprite* current = NULL;
  // test hit
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprite* s = &squares_sprites[i];
    int highest_z = 0;
    if ( x >= s->x && x < s->x+s->w && y >= s->y && y < s->y+s->h ) {
      if ( s->z > highest_z ) {
        current = s;
        highest_z = s->z;
      }
    }
  }
  
  return current;
}

void squares_move_sprites(void) {
  for(int i=0;i<SQUARES_NUM_SPRITES;i++) {
    squares_sprite* s = &squares_sprites[i];
    if ( s->x < 0 || s->x > NUM_COLUMNS ) {
      s->dx *= -1;
    }
    s->x += s->dx;

    if ( s->y < 0 || s->y > NUM_ROWS ) {
      s->dy *= -1;
    }
    s->y += s->dy;
  }
}
