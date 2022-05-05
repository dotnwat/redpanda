syschecks = library('syschecks', [
  'syschecks.cc', 'pidfile.cc'],
  dependencies : [seastar, absl],
  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
  link_with : [utils],
  implicit_include_directories : false,
  include_directories: include_dirs)
