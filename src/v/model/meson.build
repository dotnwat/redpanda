model = library('model', [
    'model.cc',
    'record_batch_reader.cc',
    'record_utils.cc',
    'async_adl_serde.cc',
    'adl_serde.cc',
    'validation.cc'],
  dependencies : [seastar, xxhash, hdrhist, roaring, hashing],
  cpp_args : '-DXXH_PRIVATE_API',
  implicit_include_directories : false,
  include_directories: include_dirs)

# should probably have direct dep on hashing via declare dep
