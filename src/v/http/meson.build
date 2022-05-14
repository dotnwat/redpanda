libhttp = library('http',
  'iobuf_body.cc',
  'chunk_encoding.cc',
  'client.cc',
  'logger.cc',
  implicit_include_directories: false,
  dependencies: [boost, common, bytes])

http = declare_dependency(
  dependencies: [boost, common, bytes],
  link_with: libhttp)
