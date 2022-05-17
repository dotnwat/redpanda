libbytes = library('bytes',
  'bytes.cc',
  'iobuf.cc',
  dependencies: [common, seastar],
  implicit_include_directories: false)

bytes = declare_dependency(
  link_with: libbytes,
  dependencies: [common, seastar, absl])

bytes_tests = [
  'iobuf_tests',
  'iobuf_utils_tests',
  'bytes_tests']

foreach ttest : bytes_tests
  t = executable(f'test_@ttest@',
    f'tests/@ttest@.cc',
    dependencies: [seastar_testing_main, bytes])
  test(ttest, t)
endforeach
