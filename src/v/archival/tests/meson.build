# ARGS "-- -c 1"
t = executable('test_archival',
  'service_fixture.cc',
  'ntp_archiver_test.cc',
  'segment_reupload_test.cc',
  'ntp_archiver_reupload_test.cc',
  'retention_strategy_test.cc',
  dependencies: [seastar_testing_main, archival, storage_test_utils,
    redpanda_app, model_test_utils, http_test_utils])
test('archival', t)
