// "chaser", by Ka-Ping Yee

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

#include "serpent.h"

unsigned char pixels[1000];

void next_frame(int frame) {
  int pos = frame % 300;
  for (int i = 0; i < 300; i++) {
    set_rgb(pixels, i,
            (i == pos) ? 255 : 0,
            (i == pos + 1) ? 255 : 0,
            (i == pos + 2) ? 255 : 0);
  }
  put_pixels(0, pixels, 300);
}

