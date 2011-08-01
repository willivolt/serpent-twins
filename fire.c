#include <stdlib.h>
#include "serpent.h"

unsigned char pixels[1000];

void next_frame(int frame) {
  int intensity;
  int transition;
  int pop;

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
