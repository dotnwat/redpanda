libbytes = library('bytes',
  'bytes.cc',
  'iobuf.cc',
  dependencies: [common, seastar],
  implicit_include_directories: false)

bytes = declare_dependency(
  link_with: libbytes,
  dependencies: [common, seastar])
