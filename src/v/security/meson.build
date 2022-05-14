libsecurity = library('security',
  'scram_algorithm.cc',
  'scram_authenticator.cc',
  'credential.cc',
  'acl.cc',
  'mtls.cc',
  'license.cc',
  implicit_include_directories: false,
  dependencies: [seastar, ctre, bytes, utils, model, json, cryptopp, random, re2])

security = declare_dependency(
  dependencies: [seastar, ctre, bytes, utils, model, json, cryptopp, random, re2],
  link_with: libsecurity)
