project('redpanda', 'cpp',
  meson_version: '>=1.00.0',
  default_options : [
    'cpp_std=c++20',
    'warning_level=1',
    'default_library=static',
  ])

subdir('tools')

add_test_setup('foo',
  exe_wrapper: [test_runner, '--meson'],
  is_default: true)

subdir('src/v')
