libfinjector = library('finjector',
  'hbadger.cc',
  dependencies : [common, seastar, absl],
  implicit_include_directories: false)

finjector = declare_dependency(
  dependencies : [common, seastar, absl],
  link_with: libfinjector)

#cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
#include_directories: include_dirs)
