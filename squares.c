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

#define NUM_SPRITES 90
#define MAX_W 16			// max width (around circ.) of a sprite
#define MAX_H 24			// max height (length of serpent) of a sprite
#define MAX_Z 100			// max # of Z-levels (could improve this by making sure no two sprites are on the same Z-level)
#define MAX_DX 2.0			// max X-axis movement in pix/frame
#define MAX_DY 7.5			// max Y-axis movement in pix/frame
#define MAX_BG_BLINK 160 	// max # of frames between flashes

typedef struct {
	int w,h,z;
	float x,y;
	int r,g,b;
	float dx, dy;
} sprite;

typedef struct {
	int r,g,b;
} color;

float sin_value(float);
int get_r_val(int, int, int);
void init_sprites(void);
sprite* top_sprite(int x, int y);
void move_sprites(void);

unsigned char pixels[NUM_PIXELS*3];
sprite sprites[NUM_SPRITES];
color bg;
int blinktimer;

void next_frame(int frame) {
	if (frame == 0) {
		init_sprites();
	}
	
	switch(blinktimer) {
		case 5:
			bg.r = bg.g = bg.b = 128;
			break;
		case 4:
			bg.r = bg.g = bg.b = 255;
			break;
		case 3:
			bg.r = bg.g = bg.b = 192;
			break;
		case 2:
			bg.r = bg.g = bg.b = 128;
			break;
		case 1:
			bg.r = bg.g = bg.b = 64;
			break;
		case 0:
			blinktimer = rand()%MAX_BG_BLINK;
			// fall through intentionally
		default:
			bg.r = bg.g = bg.b = 0;
			break;
	}
	blinktimer--;
	    
    for (int i = 0; i < NUM_PIXELS; i++) {
   		int x = (i % NUM_COLUMNS);  // theta.  0 to 24
   	    int y = (i / NUM_COLUMNS);  // along cylinder.  0 to NUM_SEGS*SEG_ROWS (120)
        // reverse every other row
        if (y % 2 == 1) {
            x = (NUM_COLUMNS-1)-x;
        }

		sprite* s = top_sprite(x,y);
		if ( NULL == s ) {
			pixels[i*3] = bg.r;
			pixels[i*3+1] = bg.g;
			pixels[i*3+2] = bg.b;
		} else {
			pixels[i*3] = s->r;
			pixels[i*3+1] = s->g;
			pixels[i*3+2] = s->b;
		}
	}
	put_pixels(0, pixels, NUM_PIXELS);
	move_sprites();
}

void init_sprites() {
	srand( time(NULL) );
	for(int i=0;i<NUM_SPRITES;i++) {
		sprites[i].w = rand()%MAX_W;
		sprites[i].h = rand()%MAX_H;
		sprites[i].z = rand()%MAX_Z;
		sprites[i].r = rand()%256;
		sprites[i].g = rand()%256;
		sprites[i].b = rand()%256;
		float r = (float)rand()/(float)RAND_MAX;
		sprites[i].x = r * (MAX_W*2+NUM_COLUMNS);
		r = (float)rand()/(float)RAND_MAX;
		sprites[i].y = r * (MAX_H*2+NUM_ROWS);
		r = (float)(rand()-rand())/(float)RAND_MAX;
		sprites[i].dx = r * MAX_DX;
		r = (float)(rand()-rand())/(float)RAND_MAX;
		sprites[i].dy = r * MAX_DX;
	}
}

sprite* top_sprite(int x, int y) {
	sprite* current = NULL;
	// test hit
	for(int i=0;i<NUM_SPRITES;i++) {
		sprite* s = &sprites[i];
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

void move_sprites(void) {
	for(int i=0;i<NUM_SPRITES;i++) {
		sprite* s = &sprites[i];
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
