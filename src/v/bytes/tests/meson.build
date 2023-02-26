exe = executable('test_bytes',
  'iobuf_tests.cc',
  'iobuf_utils_tests.cc',
  'bytes_tests.cc',
  dependencies: [seastar_testing_main, bytes, random])
test('bytes', exe)

executable('fuzz_iobuf',
  'iobuf_fuzz.cc',
  link_args: '-fsanitize=fuzzer',
  dependencies: [seastar_testing_main, bytes, random])
