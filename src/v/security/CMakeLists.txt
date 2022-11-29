v_cc_library(
  NAME security
  SRCS
    scram_algorithm.cc
    scram_authenticator.cc
    credential.cc
    acl.cc
    mtls.cc
    license.cc
    oauth_authenticator.cc
  DEPS
    v::bytes
    v::utils
    absl::flat_hash_map
    absl::flat_hash_set
    cryptopp
    re2
 )

add_subdirectory(tests)
