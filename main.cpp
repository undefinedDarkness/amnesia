#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <filesystem>
#include <thread>

#ifdef __WIN32__
#include <windows.h>
#endif

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
  // Divide code into chunks
  //  if chunk is too small then do it linearly
  //  otherwise fire up 4 threads to do the work
  size_t chunk = size / 4;

  if (chunk < 1000) {
    for (int i = 0; i < size; i++)
      pixels[i] = fn(pixels[i]);
  } else {
    size_t rem = size & 3;
    std::array<std::thread, NTHREAD> threads;
    size_t start = 0;
    for (int i = 0; i < NTHREAD; i++) {
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
  //testImpl();
  #ifdef __WIN32__
  // Hack because windows sucks and won't interpret UTF8 output in the console correctly, same deal for the VSC Console
  SetConsoleOutputCP(CP_UTF8);
  #endif

  argh::parser cmdl(argv);

  if (cmdl[{"-h", "--help"}]) {
    std::cout << R"(
AMNESIA - NES's little colourizer
---------------------------------
./[BINARY] [FILE-PATH]
--help, -h => Help message
--no-gpu => Disable GPU Compute & Go back to SIMD (MAY GET DIFFERENT RESULTS)
--no-simd => Disable SIMD & Go back to simpler code
--palette, -p => Designate palette (by default will try to read palette.hex & go back to builtin palette)
--dmethod, -dm => Specify dithering method to use (Default is bayer dithering) {Possible values: bayer, atkinson, floydsteinberg, bluenoise}
    )";
    return 0;
  }

    if (cmdl[1].empty()) {
    std::cerr << "Nothing to do" << std::endl;
    return 0;
  }

  if (!std::filesystem::exists(cmdl[1])) {
    std::cerr << "File " << cmdl[1] << " doesn't exist.\n";
    return 1;
  }

  uint8_t *pixelData;
  int width, height, channels;
  pixelData = stbi_load(cmdl[1].c_str(), &width, &height, &channels, 4);

  std::string paletteFile;
  cmdl({"-p", "--palette"}, "palette.hex") >> paletteFile;

  uint32_t paletteSize;
  uint32_t *palette = loadPalette(&paletteSize, paletteFile.c_str());

  std::string ditherMethod;
  cmdl({"-dm", "--dmethod"}, "bayer") >> ditherMethod;

  // -- MAIN WORK: File is loaded --
  // TODO: Rework this logic a little
  void* gpuInst = cmdl["--no-gpu"] ? nullptr : doGPUInit();
  if (gpuInst == nullptr) {
    std::cerr << "GPU Compute unavailable (or --no-gpu given) returning to CPU operation\n";
    if (ditherMethod == "atkinson") {
      ditherPass(0, height, width, (pixelv*)pixelData);
    }
    doNNS((uint32_t*)pixelData, width, height, palette, paletteSize);
 } else {
  std::string ss;
    std::cerr << "GPU Compute initialized sucessfully\n";
    uint8_t options = 0;
    if (ditherMethod == ("bayer")) {
      options |= GPUOptions::BDITHER;
    }
    doGPUWork(gpuInst, (uint32_t*)pixelData, width, height, options, palette, paletteSize);
  }
  doGPUDeinit(gpuInst);

  stbi_write_png("output.png", width, height, 4, pixelData, width * 4); // TODO: FIX SLOW WRITING!!!
  free(pixelData);
  std::cout << "FINISHED\n";
  return 0;
}
