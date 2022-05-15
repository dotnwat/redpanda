subdir('schemata')

libkafka_protocol = library('libkafka_protocol',
  hdrs + srcs + ['errors.cc'],
  dependencies: [seastar, hashing],
  include_directories: include_dirs)

kafka_protocol = declare_dependency(
  sources: hdrs,
  dependencies: [seastar, hashing],
  include_directories: include_dirs,
  link_with: libkafka_protocol)
