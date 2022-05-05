pandaproxy_common = library('pandaproxy_common', [
    'error.cc',
    'logger.cc',
    'probe.cc',
    'server.cc'],
  dependencies : [seastar, absl, ctre, hdrhist, config],
  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
  link_with : [bytes, kafka, utils],
  implicit_include_directories : false,
  include_directories: include_dirs)

subdir('api')
subdir('parsing')
subdir('json')
subdir('schema_registry')
subdir('rest')
