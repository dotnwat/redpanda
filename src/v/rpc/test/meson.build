rpc_test_services = [
  'cycling_service',
  'echo_service',
  'echo_v2_service',
]

rpc_test_services_h = []
foreach service : rpc_test_services
  t = custom_target(
    output: f'@service@.h',
    input: f'@service@.json',
    command: [rpc_compiler, '--service_file', '@INPUT@',
      '--output_file', '@OUTPUT@'])
  rpc_test_services_h += [t]
endforeach

exe = executable('test_rpc',
  'netbuf_tests.cc',
  'roundtrip_tests.cc',
  'response_handler_tests.cc',
  'serialization_test.cc',
  dependencies: [seastar_testing_main, rpc])
test('rpc', exe)

#
# TODO looks like this needs to have the tls inputs setup
#
# INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/redpanda.crt
#             ${CMAKE_CURRENT_SOURCE_DIR}/redpanda.key
#             ${CMAKE_CURRENT_SOURCE_DIR}/root_certificate_authority.chain_cert
#             ${CMAKE_CURRENT_SOURCE_DIR}/redpanda.other.crt
#             ${CMAKE_CURRENT_SOURCE_DIR}/redpanda.other.key
#             ${CMAKE_CURRENT_SOURCE_DIR}/root_certificate_authority.other.chain_cert
# ARGS "-- -c 1"
exe = executable('test_rpc_generator',
  'rpc_gen_cycling_test.cc',
  rpc_test_services_h,
  dependencies: [seastar_testing_main, rpc])
test('rpcx', exe,
  args: [
    '--copy_file', 'redpanda.crt',
    '--copy_file', 'redpanda.key',
    '--copy_file', 'root_certificate_authority.chain_cert',
    '--copy_file', 'redpanda.other.crt',
    '--copy_file', 'redpanda.other.key',
    '--copy_file', 'root_certificate_authority.other.chain_cert',
  ])

exe = executable('test_exponential_backoff',
  'exponential_backoff.cc',
  dependencies: [boost, rpc])
test('rpc', exe)

exe = executable('bench_rpc_serialization',
  'rpc_bench.cc',
  dependencies: [seastar_perf_testing, rpc])
benchmark('rpc', exe)
