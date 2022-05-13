http = library('http', [
    'iobuf_body.cc',
    'chunk_encoding.cc',
    'client.cc',
    'logger.cc'],
  dependencies : [seastar],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, net],
  implicit_include_directories : false,
  include_directories: include_dirs)
