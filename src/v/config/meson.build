libconfig = library('config',
  'configuration.cc',
  'node_config.cc',
  'base_property.cc',
  'rjson_serialization.cc',
  'validators.cc',
  dependencies : [common, seastar, json, absl, fmt])

config = declare_dependency(
  dependencies: [common, seastar, json, absl, fmt],
  link_with: libconfig)

config_tests = [
  'bounded_property_test',
  'enum_property_test',
  'retention_property_test',
  'config_store_test',
  'socket_address_convert_test',
  'tls_config_convert_test',
  'advertised_kafka_api_test',
  'seed_server_property_test']

foreach ttest : config_tests
  t = executable(f'test_@ttest@',
    f'tests/@ttest@.cc',
    dependencies: [config, seastar_testing_main])
  test(ttest, t)
endforeach
