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

unsigned char pixels[1000];

void next_frame(int frame) {
  int intensity;
  int transition;
  int pop;

  if(frame%2==0) {
    for(int i=0;i<300;i++) {
      intensity = rand()%90+10;
      transition = rand()%60+30;
      pop = rand()%100;

      if(pop==0) {
        pixels[i*3]=0xff;
        pixels[i*3+1]=0xff;
        pixels[i*3+2]=0xff;
      }
      else {
        pixels[i*3]=(0xff)*intensity/100;
        pixels[i*3+1]=((0xb0-0x00)*transition/100)*intensity/100;
        pixels[i*3+2]=0;
      }

    }
    for(int s = 0; s<10; s++) {
      put_pixels(s, pixels, 300);
    }
  }
}
