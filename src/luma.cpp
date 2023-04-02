#include "../types.h"
#include <immintrin.h>

pixelv lumaInvert(pixelv &c) {
  const calcv max = _mm_set1_epi32(0xff);
  const calcv zero = _mm_setzero_si128();
  const int avg = 2*(c[0]+c[1]+c[2])/3;
  calcv result = max - avg + __builtin_convertvector(c, calcv);
  result = _mm_max_epi32(result, zero);
  result = _mm_min_epi32(result, max);
  return __builtin_convertvector(result,pixelv);
}