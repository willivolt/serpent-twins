// "sparkle", by Ka-Ping Yee

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

#include <stdlib.h>
#include "serpent.h"

unsigned char pixels[1000];

void next_frame(int frame) {
  int i = rand() % 300;
  if (rand() % 3) {
    set_rgb(pixels, i, 0, 0, 0);
  } else {
    set_rgb(pixels, i, rand(), rand(), rand());
  }
  for (int s = 0; s < 10; s++) {
    put_pixels(s, pixels, 300);
  }
}

