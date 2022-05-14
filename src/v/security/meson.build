libsecurity = library('security',
  'scram_algorithm.cc',
  'scram_credential.cc',
  'scram_authenticator.cc',
  'acl_store.cc',
  'mtls.cc',
  'license.cc',
  implicit_include_directories: false,
  dependencies: [seastar, ctre, common, bytes, utils, model, json])

security = declare_dependency(
  dependencies: [seastar, ctre, common, bytes, utils, model, json],
  link_with: libsecurity)
