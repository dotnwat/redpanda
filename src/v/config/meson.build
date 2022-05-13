libconfig = library('config',
  'configuration.cc',
  'node_config.cc',
  'base_property.cc',
  'rjson_serialization.cc',
  'validators.cc',
  dependencies : [common, seastar, json])

config = declare_dependency(
  dependencies: [common, seastar, json],
  link_with: libconfig)
