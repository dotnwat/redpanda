t = executable('test_ssx',
  'tests/async_transforms.cc',
  'tests/sformat.cc',
  'tests/future_util.cc',
  dependencies: [common, seastar_testing_main])
test('ssx', t)

t = executable('bench_ssx',
  'tests/sformat_bench.cc',
  dependencies: [common, seastar_perf_testing])
benchmark('ssx', t)
