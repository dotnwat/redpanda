libs3 = library('s3',
  'client.cc',
  'signature.cc',
  'error.cc',
  'client_probe.cc',
  implicit_include_directories: false,
  dependencies: [seastar, common, bytes])

s3 = declare_dependency(
  dependencies: [seastar, common, bytes],
  link_with: libs3)
