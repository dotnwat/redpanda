# client
add_executable(s3_test_client s3_test_client_main.cc)
target_link_libraries(s3_test_client PUBLIC v::model v::rpc v::http v::s3)
set_property(TARGET s3_test_client PROPERTY POSITION_INDEPENDENT_CODE ON)

