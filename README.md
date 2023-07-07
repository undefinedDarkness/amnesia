# Amnesia - My little ditherer
A small project to fiddle around with GPU and Graphics programming, Built atop Zig and Mach

## Features
- Lookup up table (LUT) generation & application
- Dithering:
    - Bayer
    - Atkinson
    - [TODO] Floyd Steinberg
    - Blue Noise
- Nearest Neighbour Palette Lookup
- Grayscale

## Installation
### Requirements
- Zig version 0.11.0-dev.3380+7e0a02ee2:
    - linux: https://ziglang.org/builds/zig-linux-x86_64-0.11.0-dev.3380+7e0a02ee2.tar.xz
    - windows-x86_64: https://ziglang.org/builds/zig-windows-x86_64-0.11.0-dev.3380+7e0a02ee2.zip
    - macos-x86_64 (Intel): https://ziglang.org/builds/zig-macos-x86_64-0.11.0-dev.3380+7e0a02ee2.tar.xz
    - macos-aarch64 (Apple Silicon): https://ziglang.org/builds/zig-macos-aarch64-0.11.0-dev.3380+7e0a02ee2.tar.xz

```
git clone --recurse-submodules -j8 http://github.com/undefinedDarkness/amnesia
cd amnesia
zig build run -- --help
```

Example:
`./<binary> -d bayer -p palette.hex source.png`

## Credits
- [Zig](https://ziglang.org/) language & build system
- [Mach](https://machengine.org/) for native webgpu bindings
- [zig-clap](https://github.com/Hejsil/zig-clap) for argument parsing
- [stb](https://github.com/nothings/stb) for image reading / writing

## Disclaimer
Some of the math & generated images might not be completley accurate, This stuff can be kind of hard to tell