set(GOPATH ${CMAKE_CURRENT_BINARY_DIR})

# Handle how Go is found:
#
# CMAKE_GO_BINARY is not set
# CMAKE_GO_BINARY=go
# CMAKE_GO_BINARY=/path/to/go
#
if(CMAKE_GO_BINARY)
  cmake_path(GET CMAKE_GO_BINARY FILENAME CMAKE_GO_BINARY_NAME)
  cmake_path(HAS_PARENT_PATH CMAKE_GO_BINARY CMAKE_GO_BINARY_HAS_PARENT)
  if(CMAKE_GO_BINARY_HAS_PARENT)
    cmake_path(GET CMAKE_GO_BINARY PARENT_PATH CMAKE_GO_BINARY_DIR)
    find_program(GO_BINARY
      ${CMAKE_GO_BINARY_NAME}
      PATHS ${CMAKE_GO_BINARY_DIR}
      NO_DEFAULT_PATH
      REQUIRED)
  else()
    find_program(GO_BINARY ${CMAKE_GO_BINARY_NAME} REQUIRED)
  endif()
else()
  find_program(GO_BINARY go REQUIRED)
endif()

function(add_go_dependency NAME MAIN_SRC)
  get_filename_component(MAIN_SRC_ABS ${MAIN_SRC} ABSOLUTE)
  string(REPLACE "-" "_" target_name ${NAME})
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
                    COMMAND env GOPATH=${GOPATH} ${GO_BINARY} build
                    -modcacherw -o "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
                    ${CMAKE_GO_FLAGS} ${MAIN_SRC}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${MAIN_SRC}")
  add_custom_target(${target_name} DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${NAME}")
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${NAME} DESTINATION bin)
endfunction(add_go_dependency)

add_go_dependency(kafka-request-generator kafka-request-generator.go)
