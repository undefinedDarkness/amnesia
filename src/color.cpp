#include "../types.h"
#include <stdarg.h>
#include "rl.h"
// #include <bits/stdint-uintn.h>
#include <algorithm>
#include <climits>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <vector>
#include <cstdio>
#include <xmmintrin.h>
#include <cmath>

typedef unsigned char bv __attribute((vector_size(8 * sizeof(short))));
typedef short sv __attribute((vector_size(8 * sizeof(short))));
typedef double dv __attribute((vector_size(4*sizeof(double))));
#define CC(x) x //(pixelv){(x>>24)&0xff, (x>>16)&0xff, (x>>8)&0xff, x&0xff}

alignas(16) static unsigned int colors[64+16] = {
    CC(0xc65f5f00), CC(0x859e8200), CC(0xd9b27c00), CC(0x72879700),
    CC(0x99839600), CC(0x829e9b00), CC(0xab938200), CC(0xd08b6500),
    CC(0x3d383700), CC(0x25222100), CC(0x26232200), CC(0x302c2b00),
    CC(0x3d383700), CC(0x413c3a00), CC(0xc8bAA400), CC(0xcdc0ad00),
    CC(0xbeae9400), CC(0xd1c6b400)};
static size_t nColors = 18;

static short minv(__m128i x, int &i) {
  int result = _mm_cvtsi128_si32(_mm_minpos_epu16(x));
  i = result >> 16;
  return result;
}

// TODO: See if the compiler an do this faster
const auto nullify = _mm_set1_epi16(SHRT_MAX);
static __m128i findPixelDiff(__m128i &a, const __m128i &b) {
  // I tried using mpsad, it can do some of the work but not entirely
  // maybe you could still make it work with 2 shuffles.. I'm too lazy tough
  /*bv c = a - b;
  c = _mm_abs_epi8(c);
  __m128i result = (sv){ static_cast<short>(c[0]+c[1]+c[2]),
  static_cast<short>(c[4]+c[5]+c[6]), static_cast<short>(c[8]+c[9]+c[10]),
  static_cast<short>(c[12]+c[13]+c[14]), SHRT_MAX, SHRT_MAX, SHRT_MAX, SHRT_MAX
  }; return result;*/

  const auto shuffle1 = _mm_setr_epi8(0, 1, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1,
                                -1, -1, -1, -1);
  const auto shuffle2 = _mm_setr_epi8(-1, -1, -1, -1, 0, 1, 10, 11, -1, -1, -1, -1,
                                -1, -1, -1, -1);
  auto pt12 = _mm_mpsadbw_epu8(a, b, 0);
  auto pt34 =
      _mm_mpsadbw_epu8(_mm_shuffle_epi32(a, _MM_SHUFFLE(1, 0, 4, 3)), b, 0);
  pt12 = _mm_shuffle_epi8(pt12, shuffle1);
  pt34 = _mm_shuffle_epi8(pt34, shuffle2);
  auto result = _mm_blend_epi16(_mm_max_epi16(pt12, pt34), nullify, 0b11110000);
  return result;
}

pixelv findNearest(pixelv &find) {
  const __m128i sub =
      _mm_set1_epi32(find[0] << 24 | find[1] << 16 | find[2] << 8 | 0x00);

  auto iter = [=](const int s, int &minvindex) -> int {
    // load 36 elements
    __m128i a = _mm_stream_load_si128((__m128i *)&colors[s + 0]);
    __m128i b = _mm_stream_load_si128((__m128i *)&colors[s + 4]);
    __m128i c = _mm_stream_load_si128((__m128i *)&colors[s + 8]);
    __m128i d = _mm_stream_load_si128((__m128i *)&colors[s + 12]);
    __m128i e = _mm_stream_load_si128((__m128i *)&colors[s + 16]);
    __m128i f = _mm_stream_load_si128((__m128i *)&colors[s + 20]);
    __m128i g = _mm_stream_load_si128((__m128i *)&colors[s + 24]);
    __m128i h = s > 32 ? nullify : _mm_stream_load_si128((__m128i *)&colors[s + 28]);
    __m128i i = s > 32 ? nullify : _mm_stream_load_si128((__m128i *)&colors[s + 32]);

    // get difference
    a = findPixelDiff(a, sub);
    b = findPixelDiff(b, sub);
    c = findPixelDiff(c, sub);
    d = findPixelDiff(d, sub);
    e = findPixelDiff(e, sub);
    f = findPixelDiff(f, sub);
    h = findPixelDiff(h, sub);
    i = findPixelDiff(i, sub);

    int indexes[8];
    __m128i cmp = _mm_setr_epi16(minv(a, indexes[0]), minv(b, indexes[1]),
                                 minv(c, indexes[2]), minv(d, indexes[3]),
                                 minv(e, indexes[4]), minv(f, indexes[5]),
                                 minv(h, indexes[6]), minv(i, indexes[7]));
    int ti;
    auto mind = minv(cmp, ti);
    minvindex = s + indexes[ti] + 4 * ti;

    return mind;
  };
  int minvindex;
  if (nColors > 32) {
    int da, db, ai, bi;
    da = iter(0, ai);
    db = iter(0, bi);
    minvindex = da > db ? ai : bi;
  } else {
    iter(0, minvindex);
  }
  auto x = colors[minvindex];
  return pixelv{static_cast<unsigned char>((x >> 24) & 0xff),
                static_cast<unsigned char>((x >> 16) & 0xff),
                static_cast<unsigned char>((x >> 8) & 0xff),
                static_cast<unsigned char>(x & 0xff)};
}

void loadPalette() {
  int n;
  auto f = RL::LoadImage("palette.png");
  auto cols = RL::LoadImagePalette(f, 64, &n);
  for (int i = 0; i < std::min(64, n); i++) {
    auto col = cols[i];
    colors[i] = col.r << 24 | col.g << 16 | col.b << 8 | 0x00; //{col.r, col.g, col.b, col.a};
  }
  nColors = n;
  printf("Loaded colour palette of %d colors\n", n);
  RL::UnloadImagePalette(cols);
  RL::UnloadImage(f);

  for (int i = 0; i < nColors; i++) {
    printf("\x1b[38;2;%d;%d;%dm####\e[0m\t", colors[i]>>24 & 0xff, colors[i]>>16 & 0xff, colors[i]>>8 & 0xff); 
  }
  printf("\n");
}
