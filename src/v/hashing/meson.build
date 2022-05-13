libhashing = library('libhashing', ['murmur.cc'],
  dependencies : [xxhash, gnutls, config, crc32c],
  cpp_args : '-DXXH_PRIVATE_API',
  implicit_include_directories : false,
  include_directories: include_dirs)

hashing = declare_dependency(
  dependencies: [xxhash, gnutls, config, crc32c],
  include_directories: include_dirs,
  link_with: libhashing)
