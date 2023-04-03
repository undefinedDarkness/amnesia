#include <chrono>
#include <iostream>
#include <thread>
#include <functional>
#include <atomic>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "src/rl.h"
#include "types.h"

#include "src/argh.h"

const char *txt;
RL::Texture tex;
RL::Image img;

std::atomic<bool> operationComplete{false};

bool stopUpdating = false;
void gameplay() {
  RL::BeginDrawing();
  if (!stopUpdating)
    tex = RL::LoadTextureFromImage(img);
  RL::ClearBackground(RL::WHITE);
  RL::DrawTexture(tex, 0, 0, RL::WHITE);
  //RL::DrawTextureRec(tex, {static_cast<float>(img.width/2),0,600,400}, {0,0}, RL::WHITE);
  RL::DrawText(txt, 0, 0, 30, RL::RED);
  RL::EndDrawing();
  if (operationComplete && !stopUpdating) {
    tex = RL::LoadTextureFromImage(img);
    stopUpdating = true;
  }
}

void genericPass(std::function<pixelv (pixelv&)> fn, pixelv *pixels, long width, long height) {
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
    return 0; // no file given
  }
  img = RL::LoadImage(cmdl[1].c_str());
  RL::ImageFormat(&img, RL::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  auto pixels = static_cast<pixelv *>(img.data);

  if (cmdl[{"-p", "--palette" }])
    loadPalette();

  std::thread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto start = std::chrono::high_resolution_clock::now();

    if (cmdl[{"-l", "--luma"}])
      genericPass(lumaInvert, pixels, img.width,  img.height);
    
    if (cmdl[{"-d", "--dither"}])
      ditherPass(0, img.height, img.width, pixels);
    genericPass(findNearest, pixels, img.width, img.height);
    genericPass([](auto &c){
      c[3] = 0xff;
      return c;
    }, pixels, img.width, img.height);

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> time = end - start;
    txt = RL::TextFormat("Finished in %.5fms", time.count());
    printf("%s\n", txt);
    operationComplete = true;
  }).detach();

  RL::InitWindow(img.width, img.height, "Preview");

#ifndef PLATFORM_WEB
  while (!RL::WindowShouldClose()) {
    gameplay();
  }
#else
  emscripten_set_main_loop(gameplay, 0, 1);
#endif

  RL::UnloadImage(img);

  return 0;
}
