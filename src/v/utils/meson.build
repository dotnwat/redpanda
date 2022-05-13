utils = library('utils', [
    'hdr_hist.cc',
    'human.cc',
    'file_io.cc',
    'base64.cc',
    'retry_chain_node.cc'],
  dependencies : [seastar, hdrhist, base64, hashing],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes],
  implicit_include_directories : false,
  include_directories: include_dirs)
