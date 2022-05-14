pandaproxy_parsing = library('pandaproxy_parsing', [
  'error.cc'],
  dependencies : [seastar, xxhash, hdrhist, roaring, absl],
  cpp_args : '-DXXH_PRIVATE_API',
  #link_with : [pandaproxy_common, pandaproxy_parsing, pandaproxy_json, kafka_client],
  implicit_include_directories : false,
  include_directories: include_dirs)
