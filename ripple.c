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

  if(frame%16==0) {
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
    red[i]=red[i]*19/20;
    green[i]=green[i]*19/20;
    blue[i]=blue[i]*19/20;

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
