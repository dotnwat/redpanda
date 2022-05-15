libs3 = library('s3',
  'client.cc',
  'signature.cc',
  'error.cc',
  'client_probe.cc',
  implicit_include_directories: false,
  dependencies: [seastar, common, bytes, http])

s3 = declare_dependency(
  dependencies: [seastar, common, bytes, http],
  link_with: libs3)

t = executable('test_s3',
  'tests/signature_test.cc',
  'tests/s3_client_test.cc',
  dependencies: [s3, seastar_testing_main])
test('http client', t)
