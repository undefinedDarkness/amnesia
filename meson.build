project('dither-bench', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++14'])

deps = [ dependency('threads')]

exe = executable('dither_bench', 'dither_bench.cpp',
  install : true, link_args: ['-lraylib', '-ldl' , '-lGL'], dependencies : deps)

test('basic', exe)
