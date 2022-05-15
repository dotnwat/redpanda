librpc = library('rpc',
  'types.cc',
  'netbuf.cc',
  'transport.cc',
  'reconnect_transport.cc',
  'connection_cache.cc',
  'simple_protocol.cc',
  implicit_include_directories: false,
  dependencies: [common, net])

rpc = declare_dependency(
  dependencies: [common, net],
  link_with: librpc)

rpc_compiler = find_program('rpc_compiler.py')
