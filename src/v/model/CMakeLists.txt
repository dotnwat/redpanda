v_cc_library(
  NAME model
  SRCS
    model.cc
    record_batch_reader.cc
    record_utils.cc
    async_adl_serde.cc
    adl_serde.cc
    validation.cc
  DEPS
    Seastar::seastar
    v::rphashing
    v::serde
  )
add_subdirectory(tests)
