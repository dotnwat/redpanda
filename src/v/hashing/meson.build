libhashing = library('hashing',
  'murmur.cc',
  dependencies: [common],
  implicit_include_directories: false)

hashing = declare_dependency(
  dependencies: [common, crc32c, bytes],
  compile_args: '-DXXH_PRIVATE_API',
  link_with: libhashing)

foreach ttest : ['xx_tests', 'secure_tests']
  t = executable(ttest,
    f'tests/@ttest@.cc',
    dependencies: [hashing, boost])
  test(ttest, t)
endforeach

t = executable('hash_bench',
  'tests/hash_bench.cc',
  dependencies: [hashing, seastar_perf_testing])
benchmark('hashing_bench', t)
