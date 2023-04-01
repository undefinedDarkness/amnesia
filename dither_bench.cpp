#include <chrono>
#include <iostream>
#include <thread>

namespace RL {
#include <raylib.h>
}

typedef unsigned char pixelv __attribute__((vector_size(4)));
typedef float errorv __attribute((vector_size(sizeof(float) * 4)));
typedef char spixelv __attribute((vector_size(4)));

#include <immintrin.h>
#include <vector>

static size_t width;
static size_t height;
static pixelv *pixels;

#include <mutex>
std::mutex outLk;

// {{{
void dither(size_t start_h, size_t height) {
  outLk.lock();
	std::cout << "Starting at " << start_h << " for "<< height<<" pixels" << std::endl;
	outLk.unlock();
  const size_t nPixels = width * height;
  size_t i;
  for (i = start_h; i < start_h+height - 1; i++) {
    
	  // first pixel
    pixelv v = pixels[i * width + 0];
    pixelv o = v > 127;
    o[3] = v[3];
    spixelv err = v;
    err -= o;
    err >>= 3;

    pixels[i * width + 0 + 0] = o;
    pixels[i * width + 0 + 1] += 1 * err;
    pixels[i * width + 0 + 2] += 1 * err;
    pixels[(i + 1) * width + 0 + 1] += 1 * err;
    pixels[(i + 1) * width + 0 + 0] += 1 * err;
    pixels[(i + 2) * width + 0 + 0] += 1 * err;

    size_t j;
    for (j = 1; j < width - 1; j++) {
      pixelv v = pixels[i * width + j];
      pixelv o = v > 127;
      o[3] = v[3];
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
    o[3] = v[3];
    err = v;
    err -= o;
    err >>= 4;

    pixels[i * width + j + 0] = o;
    pixels[(i + 1) * width + j - 1] += 1 * err;
    pixels[(i + 1) * width + j + 0] += 1 * err;
    pixels[(i + 2) * width + j + 0] += 1 * err;
  }
}

// }}}

void grayscale() {
	
}

void dither_multi() {
	const auto hw = std::thread::hardware_concurrency();
	const size_t chunkSize = height/hw;
	const size_t remainder = height%hw;
	std::vector<std::thread> threads;
	threads.reserve(hw+1);
	int i;
	threads.emplace_back(std::thread([=](){
				dither(0, chunkSize);
				}));
	for (i = 1; i < hw-1; i++) {
		threads.emplace_back(std::thread([=](){
					dither(chunkSize*i , chunkSize);
		}));
	}
	threads.emplace_back(std::thread([=](){
					dither(chunkSize*i, remainder ? chunkSize : chunkSize-3);
		}));
	if (remainder) {
		threads.emplace_back(std::thread([=](){
					dither(chunkSize*hw, remainder-3);
		}));
	}
	for (auto&& t : threads) 
		t.join();
}

int main(int argc, char **argv) {

  auto img = RL::LoadImage("test2.png");
  /* RL::ImageColorGrayscale(&img); */
  RL::ImageFormat(&img, RL::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  width = img.width;
  height = img.height;
  pixels = static_cast<pixelv *>(img.data);

  auto start = std::chrono::high_resolution_clock::now();
dither_multi();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> time = end - start;

  RL::InitWindow(img.width, img.height, "Preview");
  auto tex = RL::LoadTextureFromImage(img);
  auto txt = RL::TextFormat("Finished in %.5fms", time.count());

  while (!RL::WindowShouldClose()) {
    RL::BeginDrawing();
    RL::ClearBackground(RL::WHITE);
    RL::DrawTexture(tex, 0, 0, RL::WHITE);
    RL::DrawText(txt, 0, 0, 70, RL::RED);
    RL::EndDrawing();
  }

  RL::UnloadImage(img);

  return 0;
}
