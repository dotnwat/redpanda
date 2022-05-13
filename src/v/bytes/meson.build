libbytes = library('bytes',
  'bytes.cc',
  'iobuf.cc',
  dependencies: [common, seastar],
  implicit_include_directories: false)

bytes = declare_dependency(
  link_with: libbytes,
  dependencies: [common, seastar, absl])

t = executable('test_bytes',
  'tests/iobuf_tests.cc',
  'tests/iobuf_utils_tests.cc',
  'tests/bytes_tests.cc',
  dependencies: [seastar_testing_main, bytes])

test('bytes test', t)
