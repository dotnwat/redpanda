vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO redpanda-data/seastar
    REF 16a722bc0a8792daa20c165dabc07e4b1e5dff26
    SHA512 0d505b25e7a3439a325de4e1b3f451df42359ea64c442e323593e3fcace533f9f97b278b0a82d71a61c8cd3c5cc35cc35bb0dd23587b0f0684ac68621d895b22
    HEAD_REF v24.2.x
    PATCHES
      fix-gnutls-lookup.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
    -DCMAKE_CXX_STANDARD=20
    -DSeastar_DEMOS=OFF
    -DSeastar_DPDK=OFF
    -DSeastar_DOCS=OFF
    -DSeastar_APPS=OFF
    -DSeastar_TESTING=OFF
    -DSeastar_API_LEVEL=6
    -DSeastar_LOGGER_COMPILE_TIME_FMT=OFF
    -DCxxSourceLocation_IMPLEMENTS_CWG2631_EXITCODE=0
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME seastar CONFIG_PATH lib/cmake/Seastar)
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
