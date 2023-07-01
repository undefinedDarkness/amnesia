#include "../types.h"
#include <ios>
#include <iostream>
#include <stdarg.h>

#include <emmintrin.h>
#include <smmintrin.h>
#include <string>
#include <tmmintrin.h>
#include <xmmintrin.h>

#include <algorithm>
#include <fstream>
#include <vector>

#include <climits>
#include <cmath>
#include <cstdio>

typedef uint16_t v8x16 __attribute((vector_size(8 * sizeof(uint16_t))));
typedef uint8_t  v16x8 __attribute((vector_size(16 * sizeof(uint8_t))));
#define CC(x) x 

alignas(16) static uint32_t colors[64 + 16] = {
    CC(0xc65f5fff), CC(0x859e82ff), CC(0xd9b27cff), CC(0x728797ff),
    CC(0x998396ff), CC(0x829e9bff), CC(0xab9382ff), CC(0xd08b65ff),
    CC(0x3d3837ff), CC(0x252221ff), CC(0x262322ff), CC(0x302c2bff),
    CC(0x3d3837ff), CC(0x413c3aff), CC(0xc8bAA4ff), CC(0xcdc0adff),
    CC(0xbeae94ff), CC(0xd1c6b4ff)};
static size_t nColors = 18;


int32_t doNNS(uint32_t *pxs, uint32_t pxw, uint32_t pxh, uint32_t *pal, uint32_t pals) {
  const size_t canProcess = pxw*pxh - (pxw*pxh % 4);
  const __m128i move_to_front_1 = _mm_setr_epi8( 0,  1,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
  const __m128i move_to_front_2 = _mm_setr_epi8(-1, -1, -1, -1,  0,  1,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1);
  const __m128i max_16b = _mm_set1_epi16(0xffff);
  for (size_t px = 0; px < canProcess; px += 4) {
    const __m128i A = _mm_loadu_si128((__m128i*)&pxs[px]);
    uint16_t minDist = 0xffff;
    uint32_t minColor = 0x663399ff;

    // TODO: Figure out how to tell mpsadbw to use the other 3 pixels in `A`
    for (int lx = 0; lx < pals; lx += 4) {
      const __m128i B = _mm_stream_load_si128((__m128i*)&pal[lx]);
      v8x16 R1 = _mm_mpsadbw_epu8(B, A, 0);
      v8x16 R2 = _mm_mpsadbw_epu8(_mm_shuffle_epi32(B, 0b00001110), A, 0); // I tried using the bit flag but no luck
      R1 = _mm_shuffle_epi8(R1, move_to_front_1);
      R2 = _mm_shuffle_epi8(R2, move_to_front_2);
      v8x16 R12 = _mm_max_epu16(R1, R2);
      R12 = _mm_blend_epi16(R12, max_16b, 0xf0);
      const v8x16 R3 = _mm_minpos_epu16(R12);
      if (R3[0] < minDist) {
        minDist = R3[0];
        minColor = pal[lx + R3[1]];
      }
    }

    pxs[px] = minColor;
      minDist = 0xffff;
   minColor = 0x663399ff;

        for (int lx = 0; lx < pals; lx += 4) {
      const __m128i B = _mm_stream_load_si128((__m128i*)&pal[lx]);
      v8x16 R1 = _mm_mpsadbw_epu8(B, A, 1);
      v8x16 R2 = _mm_mpsadbw_epu8(_mm_shuffle_epi32(B, 0b00001110), A, 1); // I tried using the bit flag but no luck
      R1 = _mm_shuffle_epi8(R1, move_to_front_1);
      R2 = _mm_shuffle_epi8(R2, move_to_front_2);
      //printf("R1: %x %x %x %x\n", R1[0], R1[1], R1[2], R1[3]);
      //printf("R2: %x %x %x %x\n", R2[0], R2[1], R2[2], R2[3]);
      v8x16 R12 = _mm_max_epu16(R1, R2);
      R12 = _mm_blend_epi16(R12, max_16b, 0xf0);
      //printf("R12: %d %d %d %d %x\n", R12[0], R12[1], R12[2], R12[3], R12[4]);
      const v8x16 R3 = _mm_minpos_epu16(R12);
      //printf("Got max dist: %d %d\n", R3[0], R3[1]);
      //printf("\n\n");
      if (R3[0] < minDist) {
        minDist = R3[0];
        minColor = pal[lx + R3[1]];
      }
    }

    pxs[px + 1] = minColor;
          minDist = 0xffff;
   minColor = 0x663399ff;

          for (int lx = 0; lx < pals; lx += 4) {
      const __m128i B = _mm_stream_load_si128((__m128i*)&pal[lx]);
      v8x16 R1 = _mm_mpsadbw_epu8(B, A, 2);
      v8x16 R2 = _mm_mpsadbw_epu8(_mm_shuffle_epi32(B, 0b00001110), A, 2); // I tried using the bit flag but no luck
      R1 = _mm_shuffle_epi8(R1, move_to_front_1);
      R2 = _mm_shuffle_epi8(R2, move_to_front_2);
      //printf("R1: %x %x %x %x\n", R1[0], R1[1], R1[2], R1[3]);
      //printf("R2: %x %x %x %x\n", R2[0], R2[1], R2[2], R2[3]);
      v8x16 R12 = _mm_max_epu16(R1, R2);
      R12 = _mm_blend_epi16(R12, max_16b, 0xf0);
      //printf("R12: %d %d %d %d %x\n", R12[0], R12[1], R12[2], R12[3], R12[4]);
      const v8x16 R3 = _mm_minpos_epu16(R12);
      //printf("Got max dist: %d %d\n", R3[0], R3[1]);
      //printf("\n\n");
      if (R3[0] < minDist) {
        minDist = R3[0];
        minColor = pal[lx + R3[1]];
      }
    }

    pxs[px + 2] = minColor;
          minDist = 0xffff;
   minColor = 0x663399ff;

              for (int lx = 0; lx < pals; lx += 4) {
      const __m128i B = _mm_stream_load_si128((__m128i*)&pal[lx]);
      v8x16 R1 = _mm_mpsadbw_epu8(B, A, 3);
      v8x16 R2 = _mm_mpsadbw_epu8(_mm_shuffle_epi32(B, 0b00001110), A, 3); // I tried using the bit flag but no luck
      R1 = _mm_shuffle_epi8(R1, move_to_front_1);
      R2 = _mm_shuffle_epi8(R2, move_to_front_2);
      //printf("R1: %x %x %x %x\n", R1[0], R1[1], R1[2], R1[3]);
      //printf("R2: %x %x %x %x\n", R2[0], R2[1], R2[2], R2[3]);
      v8x16 R12 = _mm_max_epu16(R1, R2);
      R12 = _mm_blend_epi16(R12, max_16b, 0xf0);
      //printf("R12: %d %d %d %d %x\n", R12[0], R12[1], R12[2], R12[3], R12[4]);
      const v8x16 R3 = _mm_minpos_epu16(R12);
      //printf("Got max dist: %d %d\n", R3[0], R3[1]);
      //printf("\n\n");
      if (R3[0] < minDist) {
        minDist = R3[0];
        minColor = pal[lx + R3[1]];
      }
    }

    pxs[px + 3] = minColor;
  }

  return 0;
}

int roundUp(int numToRound, int multiple) 
{
    // assert(multiple && ((multiple & (multiple - 1)) == 0));
    return (numToRound + multiple - 1) & -multiple;
}

uint32_t *loadPalette(uint32_t *n) {
  std::string line;
  union {
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    } x;
    uint32_t z;
  } color;
  color.z = 0;
  std::ifstream paletteFile("palette.hex");

  if (!paletteFile.good()) {
    std::cerr << "Encountered error reading from palette.hex reverting to default\n";
    goto skip_fl;
  }

  nColors = 0;
  while (paletteFile >> std::hex >> color.z) {
    // check if has an alpha channel
    // if (color.x.a == 0)
      color.x.a = 0xff;
    std::swap(color.x.r, color.x.b); // Idk whats going on but this was necessary
    // std::swap(color.x.g, color.x.b);
    printf("\x1b[38;2;%d;%d;%dm██████\x1b[0m (%d, %d, %d) %x\n", color.x.r,
            color.x.g, color.x.b, color.x.r, color.x.g, color.x.b, color.z);
  	colors[nColors++] = color.z;
  }
 d
  printf("\n\x1b[0m");
  printf("Loaded %lu (padding to %d) colours from palette file: palette.hex\n", nColors, roundUp(nColors, 4));
  paletteFile.close();
  
  skip_fl:
  nColors = roundUp(nColors, 4);
  *n = nColors;
  return colors;
}
