libhttp = library('http',
  'iobuf_body.cc',
  'chunk_encoding.cc',
  'client.cc',
  'logger.cc',
  implicit_include_directories: false,
  dependencies: [boost, common, bytes, net])

http = declare_dependency(
  dependencies: [boost, common, bytes, net],
  link_with: libhttp)

t = executable('test_http_client',
  'tests/http_client_test.cc',
  'tests/framing_test.cc',
  dependencies: [http, seastar_testing_main])
test('http client', t)
