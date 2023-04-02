#include <array>
#include <chrono>
#include <climits>
#include <iostream>
#include <thread>

namespace RL {
#include <raylib.h>
}

typedef unsigned char pixelv __attribute__((vector_size(4)));
typedef char spixelv __attribute((vector_size(4)));
typedef int calcv __attribute((vector_size(4 * sizeof(int))));

#include <immintrin.h>
#include <vector>

static size_t width;
static size_t height;
static pixelv *pixels;

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define CC(x)                                                                  \
  (calcv) { (x >> 24) & 0xff, (x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff }

static std::vector<calcv> colors = {
    CC(0xc65f5fff), CC(0x859e82ff), CC(0xd9b27cff), CC(0x728797ff),
    CC(0x998396ff), CC(0x829e9bff), CC(0xab9382ff), CC(0xd08b65ff),
    CC(0x3d3837ff), CC(0x252221ff), CC(0x262322ff), CC(0x302c2bff),
    CC(0x3d3837ff), CC(0x413c3aff), CC(0xc8bAA4ff), CC(0xcdc0adff),
    CC(0xbeae94ff), CC(0xd1c6b4ff)};
#include <cmath>
#define sq(x) ((x) * (x))

pixelv findNearest(pixelv &find) {
	int distance = INT_MAX;
    calcv cc;
	calcv init = __builtin_convertvector(find, calcv);
#pragma GCC unroll 16
    for (auto& color : colors) {
		calcv c = (init-color); 
		c = -c < c ? c : -c; 
		const int res = c[0]+c[1]+c[2];
        if (distance > res) {
            cc = color;
			distance = res;
		}
    }
    return __builtin_convertvector(cc, pixelv);
}

// {{{

void dither(size_t start_h, size_t height) {
  const size_t nPixels = width * height;
  size_t i;
  for (i = start_h; i < start_h + height - 3; i++) {
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

  // == LAST ROW ==
  i++;

  v = pixels[i * width + 0];
  o = v > 127;
  o[3] = v[3];
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

// }}}

const char *txt;
RL::Texture tex;

void gameplay() {
  RL::BeginDrawing();
  RL::ClearBackground(RL::WHITE);
  RL::DrawTexture(tex, 0, 0, RL::WHITE);
  RL::DrawText(txt, 0, 0, 10, RL::RED);
  RL::EndDrawing();
}

void colorize() {
	size_t size = width*height;
	size_t chunk = size/4;
	size_t rem   = size&3;
	std::array<std::thread, 5> threads;
	size_t start = 0;
	for (int i = 0; i < 4; i++) {
		threads[i] = std::thread([=](){
			for (int j = start; j < start+chunk; j++) {
				pixels[j] = findNearest(pixels[j]);
			}
		});
		start += chunk;
	}
	if (rem) {
		threads[4] = std::thread([=](){
			for (int j = start; j < start+rem; j++) {
				pixels[j] = findNearest(pixels[j]);
			}
		});
	}
	for (int i = 0; i < 4+(!!rem); i++) {
		threads[i].join();
	}
}

int main(int argc, char **argv) {

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

  auto img = RL::LoadImage("balls.png");
  RL::ImageFormat(&img, RL::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  width = img.width;
  height = img.height;
  pixels = static_cast<pixelv *>(img.data);

  auto start = std::chrono::high_resolution_clock::now();

  dither(0, img.height);

  /* for (int i = 0; i < width * height; i++) {
    pixels[i] = findNearest(pixels[i]);
  } */
  colorize();

  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> time = end - start;

  RL::InitWindow(img.width, img.height, "Preview");
  tex = RL::LoadTextureFromImage(img);
  txt = RL::TextFormat("Finished in %.5fms", time.count());
	printf("%s\n",txt);

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
