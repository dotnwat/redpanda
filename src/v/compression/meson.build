compression = library('compression', [
    'compression.cc',
    'stream_zstd.cc',
    'logger.cc',
    'snappy_standard_compressor.cc',
    'internal/snappy_java_compressor.cc',
    'internal/lz4_frame_compressor.cc',
    'internal/gzip_compressor.cc'],
  dependencies : [seastar, zstd, snappy, zlib],
  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
  link_with : [bytes],
  implicit_include_directories : false,
  include_directories: include_dirs)
