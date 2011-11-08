// "electric", by Christopher De Vries

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

#define ELECTRIC_LOCI 200

static unsigned char pixels[9000];

void electric_draw_locus(int x, int orientation, byte* pixels) {
  int i;

  for(i=(x*25+orientation)*3;i<(x*25+25)*3;i+=9) {
    pixels[i]=0x80;
    pixels[i+1]=0x00;
    pixels[i+2]=0xff;
  }
}

void next_frame(int frame) {
  static int inv_velocity[ELECTRIC_LOCI];
  static int inv_omega[ELECTRIC_LOCI];
  static int t_not[ELECTRIC_LOCI];
  static int nnext=0;
  static int nmax=0;
  int i;
  int posx;
  int orientation;

  if(frame%10==0) {
    inv_velocity[nnext]=1000/(frame%1000+1);
    inv_omega[nnext]=333/(frame%333+1);
    t_not[nnext]=frame;

    nnext++;
    if(nnext==ELECTRIC_LOCI) nnext=0;

    if(nmax<ELECTRIC_LOCI) {
      nmax++;
    }
  }

  for(i=0;i<9000;i++) {
    pixels[i]=0;
  }

  for(i=0;i<nmax;i++) {
    posx=(frame-t_not[i])/inv_velocity[i]%120;
    orientation=(frame-t_not[i])/inv_omega[i]%3;
    electric_draw_locus(posx,orientation,pixels);
  }

  for(int s = 0; s<10; s++) {
    put_segment_pixels(s, &pixels[s*900], 300);
  }
  put_head_pixels((byte*) pixels, HEAD_PIXELS);
}

