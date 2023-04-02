project('dither-bench', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++14'])

deps = [ dependency('threads')]
srcs = [
  'dither_bench.cpp',
  'src/color.cpp',
  'src/dither.cpp',
  'src/luma.cpp'
]

exe = executable('dither_bench', srcs,
  install : true, link_args: ['-lraylib', '-ldl' , '-lGL'], dependencies : deps, cpp_args : [ '-march=native', '-mtune=native' ])

test('basic', exe)
