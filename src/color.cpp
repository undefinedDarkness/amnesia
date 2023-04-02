#include "../types.h"
#include <climits>
#include <immintrin.h>
#include <vector>
#include "rl.h"

#define CC(x)                                                                  \
  calcv{ (x >> 24) & 0xff, (x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff }

static std::vector<calcv> colors = {
    CC(0xc65f5fff), CC(0x859e82ff), CC(0xd9b27cff), CC(0x728797ff),
    CC(0x998396ff), CC(0x829e9bff), CC(0xab9382ff), CC(0xd08b65ff),
    CC(0x3d3837ff), CC(0x252221ff), CC(0x262322ff), CC(0x302c2bff),
    CC(0x3d3837ff), CC(0x413c3aff), CC(0xc8bAA4ff), CC(0xcdc0adff),
    CC(0xbeae94ff), CC(0xd1c6b4ff)};


pixelv findNearest(pixelv &find) {
  int distance = INT_MAX;
  calcv cc;
  calcv init = __builtin_convertvector(find, calcv);
  for (auto &color : colors) {
    calcv c = (init - color);
    c = -c < c ? c : -c;
    const int res = c[0] + c[1] + c[2];
    if (distance > res) {
      cc = color;
      distance = res;
    }
  }
  return __builtin_convertvector(cc, pixelv);
}

void loadPalette() {
  int n;
  auto f = RL::LoadImage("palette.png");
  auto cols = RL::LoadImagePalette(f, 100, &n);
  colors.clear();
  for (int i = 0; i < n; i++) {
    auto col = cols[i];
    colors.push_back((calcv){col.r, col.g, col.b, col.a});
  }
  RL::UnloadImagePalette(cols);
  RL::UnloadImage(f);
}
