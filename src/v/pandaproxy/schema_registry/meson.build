libpandaproxy_schema_registry = library('pandaproxy_schema_registry',
  'api.cc',
  'configuration.cc',
  'handlers.cc',
  'error.cc',
  'service.cc',
  'seq_writer.cc',
  'sharded_store.cc',
  'types.cc',
  'avro.cc',
  'protobuf.cc',
  pandaproxy_api_h,
  implicit_include_directories: false,
  dependencies: [avro, common, kafka_client])

pandaproxy_schema_registry = declare_dependency(
  sources: pandaproxy_api_h,
  dependencies: [avro, common, kafka_client],
  link_with: libpandaproxy_schema_registry)
