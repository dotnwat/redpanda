v_cc_library(
  NAME rprandom
  HRDS
    fast_prng.h
    generators.h
  SRCS
    "generators.cc"
  DEPS
    Seastar::seastar
    v::bytes
    absl::hash
    absl::random_seed_sequences
)
rp_test(
  UNIT_TEST
  BINARY_NAME basic_fast_random_test
  SOURCES random_test.cc
  DEFINITIONS BOOST_TEST_DYN_LINK
  LIBRARIES v::rprandom Boost::unit_test_framework absl::flat_hash_set
  LABELS random
)
