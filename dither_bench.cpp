#include <chrono>
#include <iostream>
#include <thread>
#include <array>
#include <climits>

namespace RL {
#include <raylib.h>
}

typedef unsigned char pixelv __attribute__((vector_size(4)));
typedef char  spixelv __attribute((vector_size(4)));
typedef int  calcv __attribute((vector_size(4 * sizeof(int))));

#include <immintrin.h>
#include <vector>

static size_t width;
static size_t height;
static pixelv *pixels;


#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define CC(x) { (x >> 24) & 0xff, (x >> 16) & 0xff, (x >> 8) & 0xff, x & 0xff }

constexpr static calcv colors[] = {
	CC(0xc65f5fff),
	CC(0x859e82ff),
	CC(0xd9b27cff),
	CC(0x728797ff),
	CC(0x998396ff),
	CC(0x829e9bff),
	CC(0xab9382ff),
	CC(0xd08b65ff),
	CC(0x3d3837ff),
	CC(0x252221ff),
	CC(0x262322ff),
	CC(0x302c2bff),
	CC(0x3d3837ff),
	CC(0x413c3aff),
	CC(0xc8bAA4ff),
	CC(0xcdc0adff),
	CC(0xbeae94ff),
	CC(0xd1c6b4ff)
};



pixelv findNearest(pixelv &c) {
    int maxDist = INT_MAX;
    calcv cc;
	const calcv init = __builtin_convertvector(c,calcv);
    for (auto& color : colors) {
        calcv c = init;
		c -= color;
        c *= c;
        if (maxDist > c[0]+c[1]+c[2]) {
            cc = color;
			maxDist = c[0]+c[1]+c[2];
		}
    }
	cc[3] = init[3];
    return __builtin_convertvector(cc, pixelv);
}

// {{{

void dither(size_t start_h, size_t height) {
  const size_t nPixels = width * height;
  size_t i;
  for (i = start_h; i < start_h + height - 3; i++) {
    pixelv v = pixels[i * width + 0];
    pixelv o = findNearest(v);
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
      pixelv o = findNearest(v);
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
    o = findNearest(v);
    err = v;
    err -= o;
    err >>= 4;

    pixels[i * width + j + 0] = o;
    pixels[(i + 1) * width + j - 1] += 1 * err;
    pixels[(i + 1) * width + j + 0] += 1 * err;
    pixels[(i + 2) * width + j + 0] += 1 * err;
  }
  pixelv v = pixels[i * width + 0];
  pixelv o = findNearest(v);
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
    pixelv o = findNearest(v);
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
  o = findNearest(v);
  err = v;
  err -= o;
  err >>= 4;

  pixels[i * width + j + 0] = o;
  pixels[(i + 1) * width + j - 1] += 1 * err;
  pixels[(i + 1) * width + j + 0] += 1 * err;

  // == LAST ROW ==
  i++;

  v = pixels[i * width + 0];
  o = findNearest(v);
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
    pixelv o = findNearest(v);
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
  o = findNearest(v);
  err = v;
  err -= o;
  err >>= 4;

  pixels[i * width + j + 0] = o;
  pixels[(i + 1) * width + j - 1] += 1 * err;
}

// }}}



const char* txt;
RL::Texture tex;

void gameplay() {
    RL::BeginDrawing();
    RL::ClearBackground(RL::WHITE);
    RL::DrawTexture(tex, 0, 0, RL::WHITE);
    RL::DrawText(txt, 0, 0, 70, RL::RED);
    RL::EndDrawing();
}

int main(int argc, char **argv) {

	std::cout << "\x1b[48;2;255;255;255m";
	for (auto& color : colors) {
		std::cout << "\x1b[38;2;" << color[0] << ';' << color[1] << ';' << color[2] << "m◼   ";
	}
	std::cout << "\e[0m" << std::endl;

  auto img = RL::LoadImage("g.png");
  RL::ImageFormat(&img, RL::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

  width = img.width;
  height = img.height;
  pixels = static_cast<pixelv *>(img.data);

  auto start = std::chrono::high_resolution_clock::now();
  dither(0, img.height);
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> time = end - start;

  RL::InitWindow(img.width, img.height, "Preview");
  tex = RL::LoadTextureFromImage(img);
  txt = RL::TextFormat("Finished in %.5fms", time.count());

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
