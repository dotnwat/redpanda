libutils = library('utils',
  'hdr_hist.cc',
  'human.cc',
  'file_io.cc',
  'base64.cc',
  'retry_chain_node.cc',
  dependencies: [hdr_histogram, base64, common, bytes])

utils = declare_dependency(
  dependencies: [hdr_histogram, base64, common, bytes],
  link_with: libutils)

#  dependencies : [seastar, hdrhist, base64, hashing],
#  cpp_args : '-DXXH_PRIVATE_API',
#  link_with : [bytes],
#  implicit_include_directories : false,
#  include_directories: include_dirs)
