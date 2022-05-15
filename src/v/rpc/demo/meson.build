rpc_demo_service_h = custom_target(
  output: 'simple_service.h',
  input: 'gen.json',
  command: [rpc_compiler, '--service_file', '@INPUT@',
    '--output_file', '@OUTPUT@'])

executable('demo_rpc_server',
  'server.cc',
  rpc_demo_service_h,
  dependencies: [rpc])

executable('demo_rpc_client',
  'client.cc',
  rpc_demo_service_h,
  dependencies: [rpc])

# ARGS "-- -c 1"
t = executable('test_rpc_roundtrip_types',
  'type_tests.cc',
  rpc_demo_service_h,
  dependencies: [seastar_testing_main, rpc])
test('rpc roundtrip types', t)
