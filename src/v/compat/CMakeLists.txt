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
    absl::flat_hash_map
    v::compression)
