libsecurity = library('security',
  'scram_algorithm.cc',
  'scram_credential.cc',
  'scram_authenticator.cc',
  'acl_store.cc',
  'mtls.cc',
  'license.cc',
  implicit_include_directories: false,
  dependencies: [seastar, ctre, bytes, utils, model, json, cryptopp])

security = declare_dependency(
  dependencies: [seastar, ctre, bytes, utils, model, json, cryptopp],
  link_with: libsecurity)
