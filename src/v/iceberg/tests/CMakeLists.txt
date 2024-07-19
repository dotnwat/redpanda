find_package(Avro)

set(TESTDATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testdata")

rp_test(
  UNIT_TEST
  GTEST
  USE_CWD
  BINARY_NAME iceberg
  SOURCES
    datatypes_json_test.cc
    datatypes_test.cc
    manifest_serialization_test.cc
    schema_json_test.cc
    test_schemas.cc
  LIBRARIES
    Avro::avro
    Boost::iostreams
    v::bytes
    v::gtest_main
    v::iceberg
  INPUT_FILES
    "${TESTDATA_DIR}/iceberg_manifest.avro"
  ARGS "-- -c1"
)
