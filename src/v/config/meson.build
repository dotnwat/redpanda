libconfig = library('config', [
    'configuration.cc',
    'node_config.cc',
    'base_property.cc',
    'rjson_serialization.cc',
    'validators.cc'],
  dependencies : [seastar, absl, rapidjson, yaml],
  link_with : [json],
  implicit_include_directories : false,
  include_directories: include_dirs)

config = declare_dependency(
  link_with: libconfig,
  dependencies: [seastar, absl, rapidjson, yaml],
  include_directories: include_dirs)
