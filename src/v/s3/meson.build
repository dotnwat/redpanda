
s3 = library('s3', [
    'client.cc',
    'signature.cc',
    'error.cc',
    'client_probe.cc'],
  dependencies : [seastar],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, net, http],
  implicit_include_directories : false,
  include_directories: include_dirs)
