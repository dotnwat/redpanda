libpandaproxy_common = library('pandaproxy_common',
  'error.cc',
  'logger.cc',
  'probe.cc',
  'server.cc',
  implicit_include_directories: false,
  dependencies: [config, kafka_client, utils])

pandaproxy_common = declare_dependency(
  dependencies: [config, kafka_client, utils],
  link_with: libpandaproxy_common)

subdir('api/api-doc')
subdir('parsing')
subdir('json')
subdir('schema_registry')
subdir('rest')

t = executable('test_pandaproxy_unit',
  'test/errors.cc',
  dependencies: [pandaproxy_common, pandaproxy_parsing, pandaproxy_json, kafka_protocol])
test('pandaproxy unit', t)
