libsecurity = library('security',
  'scram_algorithm.cc',
  'scram_credential.cc',
  'scram_authenticator.cc',
  'acl_store.cc',
  implicit_include_directories: false,
  dependencies: [seastar, ctre, common, bytes])

security = declare_dependency(
  dependencies: [seastar, ctre, common, bytes],
  link_with: libsecurity)
