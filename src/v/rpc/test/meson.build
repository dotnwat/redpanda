cycling_service_h = custom_target(
  output : 'cycling_service.h',
  input : 'cycling_service.json',
  command : [rpc_compiler, '--service_file', '@INPUT@',
    '--output_file', '@OUTPUT@'])

echo_service_h = custom_target(
  output : 'echo_service.h',
  input : 'echo_service.json',
  command : [rpc_compiler, '--service_file', '@INPUT@',
    '--output_file', '@OUTPUT@'])

t = executable('test_rpc',
  'netbuf_tests.cc',
  'roundtrip_tests.cc',
  'response_handler_tests.cc',
  'serialization_test.cc',
  dependencies: [rpc, seastar_testing_main])
test('rpc', t)

#
# TODO looks like this needs to have the tls inputs setup
#
t = executable('test_rpc_generator',
  'rpc_gen_cycling_test.cc',
  cycling_service_h,
  echo_service_h,
  dependencies: [rpc, seastar_testing_main])
test('rpc generator', t)

t = executable('test_exponential_backoff',
  'exponential_backoff.cc',
  dependencies: [boost, rpc])
test('exponential backoff', t)

t = executable('bench_rpc_serialization',
  'rpc_bench.cc',
  dependencies: [rpc, seastar_perf_testing])
benchmark('rpc serialization', t)
