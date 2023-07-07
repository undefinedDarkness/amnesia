#include <stdint.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>
#include <xmmintrin.h>

#include <limits.h>
#include <math.h>
#include <stdio.h>

typedef uint16_t v8x16 __attribute((vector_size(8 * sizeof(uint16_t))));
typedef uint8_t  v16x8 __attribute((vector_size(16 * sizeof(uint8_t))));
#define CC(x) x 

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

typedef unsigned char pixelv __attribute__((vector_size(4)));
typedef char spixelv __attribute((vector_size(4)));
typedef int calcv __attribute((vector_size(4 * sizeof(int))));

void atkinsonDither(uint32_t start_h, uint32_t height, uint32_t width, uint8_t *pixels_ptr) {
  pixelv *pixels = pixels_ptr;
  // Not sure why I unwrapped the loop but umm yeah
  // Pretty sure this is atkinson dithering
  long i;
  for (i = start_h; i < start_h + height - 3; i++) {
    pixelv v = pixels[i * width + 0];
    pixelv o = v > 127;
    spixelv err = v;
    err -= o;
    err >>= 3;

    pixels[i * width + 0 + 0] = o;
    pixels[i * width + 0 + 1] += 1 * err;
    pixels[i * width + 0 + 2] += 1 * err;
    pixels[(i + 1) * width + 0 + 1] += 1 * err;
    pixels[(i + 1) * width + 0 + 0] += 1 * err;
    pixels[(i + 2) * width + 0 + 0] += 1 * err;

    long j;
    for (j = 1; j < width - 1; j++) {
      pixelv v = pixels[i * width + j];
      pixelv o = v > 127;
      spixelv err = v;
      err -= o;
      err >>= 3;

      pixels[i * width + j + 0] = o;
      pixels[i * width + j + 1] += 1 * err;
      pixels[i * width + j + 2] += 1 * err;
      pixels[(i + 1) * width + j - 1] += 1 * err;
      pixels[(i + 1) * width + j + 0] += 1 * err;
      pixels[(i + 1) * width + j + 1] += 1 * err;
      pixels[(i + 2) * width + j + 0] += 1 * err;
    }

    j++;
    v = pixels[i * width + j];
    o = v > 127;
    err = v;
    err -= o;
    err >>= 4;

    pixels[i * width + j + 0] = o;
    pixels[(i + 1) * width + j - 1] += 1 * err;
    pixels[(i + 1) * width + j + 0] += 1 * err;
    pixels[(i + 2) * width + j + 0] += 1 * err;
  }
  pixelv v = pixels[i * width + 0];
  pixelv o = v > 127;
  spixelv err = v;
  err -= o;
  err >>= 3;

  pixels[i * width + 0 + 0] = o;
  pixels[i * width + 0 + 1] += 1 * err;
  pixels[i * width + 0 + 2] += 1 * err;
  pixels[(i + 1) * width + 0 + 1] += 1 * err;
  pixels[(i + 1) * width + 0 + 0] += 1 * err;
  pixels[(i + 2) * width + 0 + 0] += 1 * err;

  long j;
  for (j = 1; j < width - 1; j++) {
    pixelv v = pixels[i * width + j];
    pixelv o = v > 127;
    spixelv err = v;
    err -= o;
    err >>= 3;

    pixels[i * width + j + 0] = o;
    pixels[i * width + j + 1] += 1 * err;
    pixels[i * width + j + 2] += 1 * err;
    pixels[(i + 1) * width + j - 1] += 1 * err;
    pixels[(i + 1) * width + j + 0] += 1 * err;
    pixels[(i + 1) * width + j + 1] += 1 * err;
    pixels[(i + 2) * width + j + 0] += 1 * err;
  }

  j++;
  v = pixels[i * width + j];
  o = v > 127;
  err = v;
  err -= o;
  err >>= 4;

  pixels[i * width + j + 0] = o;
  pixels[(i + 1) * width + j - 1] += 1 * err;
  pixels[(i + 1) * width + j + 0] += 1 * err;

  // == LAST ROW ==
  i++;

  v = pixels[i * width + 0];
  o = v > 127;
  err = v;
  err -= o;
  err >>= 3;

  pixels[i * width + 0 + 0] = o;
  pixels[i * width + 0 + 1] += 1 * err;
  pixels[i * width + 0 + 2] += 1 * err;
  pixels[(i + 1) * width + 0 + 1] += 1 * err;
  pixels[(i + 1) * width + 0 + 0] += 1 * err;

  for (j = 1; j < width - 1; j++) {
    pixelv v = pixels[i * width + j];
    pixelv o = v > 127;
    spixelv err = v;
    err -= o;
    err >>= 3;

    pixels[i * width + j + 0] = o;
    pixels[i * width + j + 1] += 1 * err;
    pixels[i * width + j + 2] += 1 * err;
    pixels[(i + 1) * width + j - 1] += 1 * err;
    pixels[(i + 1) * width + j + 0] += 1 * err;
    pixels[(i + 1) * width + j + 1] += 1 * err;
  }

  j++;
  v = pixels[i * width + j];
  o = v > 127;
  o[3] = v[3];
  err = v;
  err -= o;
  err >>= 4;

  pixels[i * width + j + 0] = o;
  pixels[(i + 1) * width + j - 1] += 1 * err;
}

// Little bit of glue code
uint8_t *loadImageRGBA(const uint8_t *const path, uint32_t *width, uint32_t *height) {
  uint32_t ch;
  return stbi_load(path, width, height, &ch, 4);
}

int32_t writeImageRGBA(const uint8_t *filePath, const uint8_t *image, uint32_t width, uint32_t height, uint32_t stride) {
  return stbi_write_png(filePath, width, height, 4, image, stride) == 0 ? -1 : 0;
}

void unloadImageRGBA(uint8_t *image) {
  free(image);
}

int32_t doNNS(uint32_t *pxs, uint32_t pxw, uint32_t pxh, uint8_t options, uint32_t *pal, uint32_t pals) {
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