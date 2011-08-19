// "ripple", by Christopher De Vries

// Copyright 2011 Christopher De Vries
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
#include "serpent.h"

#define RIPPLES 10
static int posx[RIPPLES];
static int posy[RIPPLES];
static int radius[RIPPLES];
static unsigned char red[RIPPLES];
static unsigned char green[RIPPLES];
static unsigned char blue[RIPPLES];

static unsigned char pixels[9000];

int pixel_red(int x, int y);

void next_frame(int frame) {
  static int nnext=0;
  static int nmax=0;
  int i,j,k;
  int distance;
  int rmin;
  int rmax;
  int pixnum;

  if(frame%22==0) {
    red[nnext]=rand()%100+56;
    green[nnext]=rand()%100+56;
    blue[nnext]=rand()%100+56;
    radius[nnext]=0;
    posx[nnext]=rand()%120;
    posy[nnext]=rand()%25+50;

    nnext++;
    if(nnext==RIPPLES) nnext=0;

    if(nmax<RIPPLES) {
      nmax++;
    }
  }

  for(i=0;i<nmax;i++) {
    radius[i]+=1;
    red[i]=red[i]*49/50;
    green[i]=green[i]*49/50;
    blue[i]=blue[i]*49/50;

  }

  for(j=0;i<9000;i++) {
    pixels[i]=0;
  }

  for(k=0;k<nmax;k++) {
    if(radius[k]/6==0) {
      rmin=0;
      rmax=radius[k]*radius[k];
    }
    else {
      rmin=radius[k]-radius[k]/6;
      rmin*=rmin;
      rmax=radius[k]+radius[k]/6;
      rmax*=rmax;
    }

    for(j=0;j<125;j++) {
      for(i=0;i<120;i++) {
        distance = (posx[k]-i)*(posx[k]-i)+(posy[k]-j)*(posy[k]-j);
        if(distance>=rmin && distance<=rmax) {
          pixnum = pixel_red(i,j%25);
          if(255-pixels[pixnum]<red[k]) {
           pixels[pixnum]=255;
          }
          else {
            pixels[pixnum]+=red[k];
          }
          pixnum++;

          if(255-pixels[pixnum]<green[k]) {
           pixels[pixnum]=255;
          }
          else {
            pixels[pixnum]+=green[k];
          }
          pixnum++;


          if(255-pixels[pixnum]<blue[k]) {
           pixels[pixnum]=255;
          }
          else {
            pixels[pixnum]+=blue[k];
          }
        }
      }
    }
  }

  for(int s = 0; s<10; s++) {
    put_pixels(s, &pixels[s*900], 300);
  }
}

int pixel_red(int x, int y) {
  return (x*25+y)*3;
}

int pixel_green(int x, int y) {
  return (x*25+y)*3+1;
}

int pixel_blue(int x, int y) {
  return (x*25+y)*3+2;
}
