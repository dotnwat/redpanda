v_cc_library(
  NAME resource_mgmt
  SRCS
    available_memory.cc
    storage.cc
  DEPS
    Seastar::seastar
    v::ssx
    v::config
  )

add_subdirectory(tests)
