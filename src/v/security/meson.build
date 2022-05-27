libsecurity = library('security',
  'scram_algorithm.cc',
  'scram_credential.cc',
  'scram_authenticator.cc',
  'acl_store.cc',
  'mtls.cc',
  implicit_include_directories: false,
  dependencies: [seastar, ctre, common, bytes, utils, model])

security = declare_dependency(
  dependencies: [seastar, ctre, common, bytes, utils, model],
  link_with: libsecurity)

t = executable('test_security',
  'tests/scram_algorithm_test.cc',
  'tests/credential_store_test.cc',
  'tests/authorizer_test.cc',
  'tests/mtls_test.cc',
  dependencies: [security, boost])
test('security', t)
