libkafka_client = library('libkafka_client',
  'assignment_plans.cc',
  'broker.cc',
  'brokers.cc',
  'client.cc',
  'client_fetch_batch_reader.cc',
  'configuration.cc',
  'consumer.cc',
  'fetcher.cc',
  'fetch_session.cc',
  'partitioners.cc',
  'producer.cc',
  'topic_cache.cc',
  'sasl_client.cc',
  dependencies : [seastar, config, cluster, xxhash, hdrhist, roaring, absl, kafka_protocol],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, model, storage, security, kafka],
  implicit_include_directories : false,
  include_directories: include_dirs)

kafka_client = declare_dependency(
  link_with: libkafka_client,
  dependencies: [seastar, config, kafka_protocol],
  include_directories: include_dirs)
