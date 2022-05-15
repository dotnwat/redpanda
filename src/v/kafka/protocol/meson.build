subdir('schemata')

libkafka_protocol = library('kafka_protocol',
  'errors.cc',
  kafka_protocol_h,
  kafka_protocol_cc,
  dependencies: [bytes, model],
  implicit_include_directories: false)

kafka_protocol = declare_dependency(
  sources: kafka_protocol_h,
  dependencies: [bytes, model],
  link_with: libkafka_protocol)
