base64 = library('base64',
  objects : ['/usr/local/lib/libbase64.o'])

utils = library('utils', [
    'hdr_hist.cc',
    'human.cc',
    'file_io.cc',
    'base64.cc',
    'retry_chain_node.cc'],
  dependencies : [seastar, hdrhist],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, hashing, base64],
  implicit_include_directories : false,
  include_directories: include_dirs)
