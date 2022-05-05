net = library('net', [
    'batched_output_stream.cc',
    'dns.cc',
    'transport.cc',
    'connection.cc',
    'conn_quota.cc',
    'server.cc',
    'probes.cc',
    'tls.cc'],
  dependencies : [seastar, absl, hdrhist, config],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, hashing],
  implicit_include_directories : false,
  include_directories: include_dirs)
