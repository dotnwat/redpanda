#
# cmake audit sha1s used to minimize changes to cmake files when rebasing on top
# of upstream. the last sha1 is the latest. append a new sha1 afer rebasing and
# updating the build.
#
# 9a397aa79
# 0ef2100f7
# 6289339c9
#
project('redpanda', 'cpp',
  meson_version: '>=1.00.0',
  default_options : [
    'cpp_std=c++20',
    'warning_level=1',
    'default_library=static',
  ])

subdir('tools')
subdir('src')
