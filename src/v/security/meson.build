security = library('security', [
    'scram_algorithm.cc',
    'scram_credential.cc',
    'scram_authenticator.cc',
    'acl_store.cc'],
  dependencies : [seastar, absl, ctre],
  cpp_args : ['-DXXH_PRIVATE_API', '-DZSTD_STATIC_LINKING_ONLY'],
  link_with : [bytes],
  implicit_include_directories : false,
  include_directories: include_dirs)
