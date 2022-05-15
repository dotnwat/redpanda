libcloud_storage = library('cloud_storage',
  'cache_service.cc',
  'access_time_tracker.cc',
  'cache_probe.cc',
  'topic_manifest.cc',
  'partition_manifest.cc',
  'recursive_directory_walker.cc',
  'remote.cc',
  'offset_translation_layer.cc',
  'probe.cc',
  'partition_probe.cc',
  'partition_recovery_manager.cc',
  'types.cc',
  'remote_segment.cc',
  'remote_partition.cc',
  'remote_segment_index.cc',
  'tx_range_manifest.cc',
  'materialized_segments.cc',
  'segment_state.cc',
  implicit_include_directories: false,
  dependencies: [seastar, config, model, cloud_storage_clients, storage])

cloud_storage = declare_dependency(
  dependencies: [seastar, config, model, cloud_storage_clients, storage],
  link_with: libcloud_storage)

exe = executable('test_cloud_storage_basic',
  'tests/directory_walker_test.cc',
  'tests/partition_manifest_test.cc',
  'tests/topic_manifest_test.cc',
  'tests/tx_range_manifest_test.cc',
  'tests/offset_translation_layer_test.cc',
  'tests/remote_segment_index_test.cc',
  dependencies: [seastar_testing_main, cloud_storage, storage_test_utils,
    model_test_utils])
test('cloud_storage', exe)

exe = executable('test_cloud_storage',
  'tests/s3_imposter.cc',
  'tests/remote_test.cc',
  'tests/remote_segment_test.cc',
  'tests/remote_partition_test.cc',
  dependencies: [seastar_testing_main, cloud_storage, storage_test_utils,
    model_test_utils])
test('cloud_storage', exe)

exe = executable('test_cloud_storage_smp',
  'tests/cache_test.cc',
  dependencies: [seastar_testing_main, cloud_storage, storage_test_utils,
    model_test_utils])
test('cloud_storage', exe)

exe = executable('bench_cloud_storage',
  'tests/cache_bench.cc',
  dependencies: [seastar_perf_testing, cloud_storage, storage_test_utils,
    model_test_utils])
benchmark('cloud_storage', exe)
