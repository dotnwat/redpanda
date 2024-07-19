find_package(Avro)

set(test_data ${CMAKE_CURRENT_BINARY_DIR}/nested_manifest.avro)

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
    ${test_data}
  ARGS "-- -c1"
)

set(generator "${PROJECT_SOURCE_DIR}/tools/gen_test_iceberg_manifest.py")
add_test(NAME generate_test_data COMMAND ${generator} -o ${test_data} -n 100)
set_tests_properties(generate_test_data PROPERTIES FIXTURES_SETUP test_fixture)
set_tests_properties(iceberg_rpunit PROPERTIES FIXTURES_REQUIRED test_fixture)
