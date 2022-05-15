libpandaproxy_rest = library('pandaproxy_rest',
  'configuration.cc',
  'handlers.cc',
  'proxy.cc',
  pandaproxy_api_h,
  implicit_include_directories: false,
  dependencies: [config, kafka_client])

pandaproxy_rest = declare_dependency(
  sources: pandaproxy_api_h,
  dependencies: [config, kafka_client],
  link_with: libpandaproxy_rest)
