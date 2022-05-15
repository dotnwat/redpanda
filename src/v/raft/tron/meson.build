tron_service_h = custom_target(
  output: 'trongen_service.h',
  input: 'gen.json',
  command: [rpc_compiler, '--service_file', '@INPUT@',
    '--output_file', '@OUTPUT@'])

executable('demo_tron_server',
  'server.cc',
  'logger.cc',
  tron_service_h,
  dependencies: [raft])

executable('demo_tron_client',
  'client.cc',
  'logger.cc',
  tron_service_h,
  dependencies: [raft])
