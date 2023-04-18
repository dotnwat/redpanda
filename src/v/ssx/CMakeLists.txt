v_cc_library(
  NAME
    ssx
  HDRS
    "future-util.h"
    "metrics.h"
  DEPS
    Seastar::seastar
    v::utils
  )

add_subdirectory(tests)
