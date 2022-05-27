ssx = declare_dependency(
  dependencies: [utils])

exe = executable('test_ssx',
  'tests/async_transforms.cc',
  'tests/sformat.cc',
  'tests/future_util.cc',
  implicit_include_directories: false,
  dependencies: [seastar_testing_main, ssx])
test('ssx', exe)

exe = executable('bench_ssx',
  'tests/sformat_bench.cc',
  implicit_include_directories: false,
  dependencies: [seastar_perf_testing, ssx])
benchmark('ssx', exe)
