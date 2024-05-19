set(VCPKG_POLICY_EMPTY_PACKAGE enabled)
set(VCPKG_BUILD_TYPE release)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL x64)
  set(TINYGO_TARBALL tinygo-linux-amd64.tar.gz)
  set(TINYGO_SHA512 7859e949dcff4105894cb7656d56ecf27b4f786ecdd06c1817e709cce6eef510e59ba411d632b3ab50d71d68b115bfbad83e8f3e9b48dff59967371ab30860aa)
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL arm64)
  set(TINYGO_TARBALL tinygo-linux-arm64.tar.gz)
  set(TINYGO_SHA512 1463020dabe44681174b8dc7c6424a603a2235f1273289913bcb914ccbadd42a5e8aabe63bf478d63d90d4095385a801defadf4e56029c29249dfdc074ac137e)
else()
  message(FATAL_ERROR "Unsupported target arch: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

vcpkg_download_distfile(tarball
  URLS https://github.com/redpanda-data/tinygo/releases/download/v0.31.0-rpk2/${TINYGO_TARBALL}
  FILENAME ${TINYGO_TARBALL}
  SHA512 ${TINYGO_SHA512}
)

vcpkg_extract_source_archive(SOURCE_PATH
  ARCHIVE ${tarball}
  SOURCE_BASE v0.31.0-rpk2
)

file(
  COPY ${SOURCE_PATH}/bin/tinygo
  DESTINATION ${CURRENT_PACKAGES_DIR}/tools/tinygo)
