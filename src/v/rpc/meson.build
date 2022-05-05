# todo: not sure why we need hdrhist dep here to get include directories
# since we already link with utils which depends on hdrhist

rpc = library('rpc', [
    'types.cc',
    'netbuf.cc',
    'transport.cc',
    'reconnect_transport.cc',
    'connection_cache.cc',
    'simple_protocol.cc'],
  dependencies : [seastar, zstd, absl, hdrhist, config],
  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
  link_with : [bytes, net, compression, hashing, utils],
  implicit_include_directories : false,
  include_directories: include_dirs)
