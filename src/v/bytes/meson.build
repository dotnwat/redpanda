bytes = library('bytes', [
    'bytes.cc',
    'iobuf.cc'],
  dependencies : [seastar, xxhash, hdrhist, roaring],
  cpp_args : '-DXXH_PRIVATE_API',
  implicit_include_directories : false,
  include_directories: include_dirs)

t = executable('test_bytes',
  'tests/iobuf_tests.cc',
  'tests/iobuf_utils_tests.cc',
  'tests/bytes_tests.cc',
  include_directories: include_dirs,
  dependencies : [seastar, absl, boost, config, numa, hwloc],
  link_with : [seastar_testing_main, bytes]
)

test('test_bytes', t)
