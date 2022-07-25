v_cc_library(
  NAME compat
  SRCS
    run.cc
    raft_compat.cc
  DEPS
    Seastar::seastar
    v::bytes
    v::rphashing
    v::utils
    v::reflection
    v::raft
    absl::flat_hash_map
    v::compression)

rp_test(
  UNIT_TEST
  BINARY_NAME test_compat
  SOURCES compat_test.cc
  LIBRARIES v::seastar_testing_main v::compat)
