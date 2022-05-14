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

t = executable('test_net_conn_rate',
  'tests/connection_rate_test.cc',
  dependencies: [net, seastar_testing_main])
test('test conn rate', t)

t = executable('test_conn_quota',
  'tests/conn_quota_test.cc',
  dependencies: [net, seastar_testing_main])
test('test conn quota', t)
