libnet = library('net',
  'batched_output_stream.cc',
  'dns.cc',
  'transport.cc',
  'connection.cc',
  'conn_quota.cc',
  'server.cc',
  'probes.cc',
  'tls.cc',
  implicit_include_directories: false,
  dependencies: [seastar, common, config, hashing, utils])

net = declare_dependency(
  dependencies: [seastar, common, config, hashing, utils],
  link_with: libnet)
