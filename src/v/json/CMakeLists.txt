v_cc_library(
  NAME json
  SRCS
    json.cc
  DEPS
    Seastar::seastar
    rapidjson
    v::hashing
)

add_subdirectory(tests)
