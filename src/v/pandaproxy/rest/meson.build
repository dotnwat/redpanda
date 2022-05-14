pandaproxy_rest = library('pandaproxy_rest', [
    'configuration.cc',
    'handlers.cc',
    'proxy.cc',
    pp_swags],
  dependencies : [seastar, xxhash, hdrhist, roaring, absl, kafka_client],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [pandaproxy_common, pandaproxy_parsing, pandaproxy_json],
  implicit_include_directories : false,
  include_directories: include_dirs)
