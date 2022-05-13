libbytes = library('bytes',
  'bytes.cc',
  'iobuf.cc',
  dependencies: [common, seastar],
  implicit_include_directories: false)

bytes = declare_dependency(
  link_with: libbytes,
  dependencies: [common, seastar, absl])

bytes_tests = [
  'tests/iobuf_tests.cc',
  'tests/iobuf_utils_tests.cc',
  'tests/bytes_tests.cc']

foreach ttest : bytes_tests
  name = ttest.underscorify()
  t = executable(name,
    ttest,
    dependencies: [seastar_testing_main, bytes])
  test(name, t)
endforeach
