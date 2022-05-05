v8_engine = library('v8_engine', [
    'data_policy.cc',
    'data_policy_table.cc'],
  dependencies : [seastar],
  cpp_args : '-DXXH_PRIVATE_API',
  implicit_include_directories : false,
  include_directories: include_dirs)
