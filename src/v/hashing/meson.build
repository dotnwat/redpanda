hashing = library('hashing', ['murmur.cc'],
  dependencies : [xxhash, gnutls, config],
  cpp_args : '-DXXH_PRIVATE_API',
  link_with : [bytes, model],
  implicit_include_directories : false,
  include_directories: include_dirs)
