libbytes = library('bytes',
  'bytes.cc',
  'iobuf.cc',
  dependencies: [common, seastar],
  implicit_include_directories: false)

bytes = declare_dependency(
  link_with: libbytes,
  dependencies: [common, seastar])

t = executable('test_bytes',
  'tests/iobuf_tests.cc',
  'tests/iobuf_utils_tests.cc',
  'tests/bytes_tests.cc',
  include_directories: include_dirs,
  dependencies : [seastar, absl, boost, config, numa, hwloc, hashing],
  link_with : [seastar_testing_main, bytes]
)

test('test_bytes', t)
