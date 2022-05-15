pandaproxy_json = library('pandaproxy_json', [
  'error.cc'],
  dependencies : [seastar, xxhash, hdrhist, roaring, absl],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [pandaproxy_common],
  implicit_include_directories : false,
  include_directories: include_dirs)
