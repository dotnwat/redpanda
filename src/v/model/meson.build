libmodel = library('model',
  'model.cc',
  'record_batch_reader.cc',
  'record_utils.cc',
  'async_adl_serde.cc',
  'adl_serde.cc',
  'validation.cc',
  implicit_include_directories: false,
  dependencies: [seastar, common, hashing])

model = declare_dependency(
  dependencies: [seastar, common, hashing],
  link_with: libmodel)
