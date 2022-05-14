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
