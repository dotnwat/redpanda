find_library(WASMTIME_LIBRARY NAMES wasmtime)

find_path(WASMTIME_INCLUDE_DIR NAMES wasmtime.h)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Wasmtime
  REQUIRED_VARS
    WASMTIME_LIBRARY
    WASMTIME_INCLUDE_DIR)

if (Wasmtime_FOUND)
  if (NOT (TARGET Wasmtime::wasmtime))
    add_library(Wasmtime::wasmtime UNKNOWN IMPORTED)

    set_target_properties(Wasmtime::wasmtime
      PROPERTIES
        IMPORTED_LOCATION ${WASMTIME_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${WASMTIME_INCLUDE_DIR})
  endif ()
endif ()
