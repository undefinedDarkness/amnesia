#include "../types.h"

void ditherPass(long start_h, long height, long width, pixelv *pixels) {
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
