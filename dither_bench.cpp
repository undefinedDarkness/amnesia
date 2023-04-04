#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>

#include "types.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/argh.h"
#include "libs/stb_image.h"
#include "libs/stb_image_write.h"

void genericPass(std::function<pixelv(pixelv &)> fn, pixelv *pixels, long width,
                 long height) {
  size_t size = width * height;
#ifdef NDEBUG
  size_t chunk = size / 4;
  if (chunk < 1000) {
    for (int i = 0; i < size; i++)
      pixels[i] = fn(pixels[i]);
  } else {
    size_t rem = size & 3;
    std::array<std::thread, 5> threads;
    size_t start = 0;
    for (int i = 0; i < 4; i++) {
      threads[i] = std::thread([=]() {
        for (int j = start; j < start + chunk; j++) {
          pixels[j] = fn(pixels[j]);
        }
      });
      start += chunk;
    }
    if (rem) {
      threads[4] = std::thread([=]() {
        for (int j = start; j < start + rem; j++) {
          pixels[j] = fn(pixels[j]);
        }
      });
    }
    for (int i = 0; i < 4 + (!!rem); i++) {
      threads[i].join();
    }
  }
#else
  for (int i = 0; i < size; i++) {
    pixels[i] = fn(pixels[i]);
  }
#endif
}

int main(int argc, char **argv) {
  argh::parser cmdl(argv);

  if (cmdl[1].empty()) {
    std::cout << "Nothing to do" << std::endl;
    return 0;
  }

  unsigned char *pixelData;
  int width, height, channels;
  pixelData = stbi_load(cmdl[1].c_str(), &width, &height, &channels, 4);
  pixelv *pixels = (pixelv *)pixelData;

  if (cmdl[{"-p", "--palette"}])
    loadPalette();

  auto start = std::chrono::high_resolution_clock::now();

  if (cmdl[{"-l", "--luma"}])
    genericPass(lumaInvert, pixels, width, height);

  if (cmdl[{"-d", "--dither"}])
    ditherPass(0, height, width, pixels);

  genericPass(findNearest, pixels, width, height);
  
  genericPass(
      [](auto &c) {
        c[3] = 0xff;
        return c;
      },
      pixels, width, height);

  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> time = end - start;
  printf("Finished in %.5fms\n", time.count());

  stbi_write_png("output.png", width, height, 4, pixelData, width * 4);
  
  if (cmdl[{"--preview"}])
    system("convert output.png sixel:");

  free(pixelData);
  return 0;
}
