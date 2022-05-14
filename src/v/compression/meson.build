libcompression = library('compression',
  'compression.cc',
  'stream_zstd.cc',
  'logger.cc',
  'snappy_standard_compressor.cc',
  'internal/snappy_java_compressor.cc',
  'internal/lz4_frame_compressor.cc',
  'internal/gzip_compressor.cc',
  cpp_args : ['-DZSTD_STATIC_LINKING_ONLY'],
  implicit_include_directories: false,
  dependencies: [common, bytes])

compression = declare_dependency(
  dependencies: [common, bytes],
  link_with: libcompression)

#  dependencies : [seastar, zstd, snappy, zlib],
#  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
#  link_with : [bytes],
#  include_directories: include_dirs)
