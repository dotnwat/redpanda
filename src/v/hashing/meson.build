libhashing = library('hashing',
  'murmur.cc',
  dependencies: [common],
  implicit_include_directories: false)

hashing = declare_dependency(
  dependencies: [common],
  link_with: libhashing)

# include_directories: include_dirs,
# dependencies: [xxhash, gnutls, config, crc32c],
#dependencies : [xxhash, gnutls, config, crc32c],
#  dependencies : [xxhash, gnutls, config, crc32c],
#  cpp_args : '-DXXH_PRIVATE_API',
