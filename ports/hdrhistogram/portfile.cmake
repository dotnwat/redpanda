vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO HdrHistogram/HdrHistogram_c
    REF b083efd27a51150201821ef6f18e0eba1f4b59f1
    SHA512 bb4396cef7ce93c9716976b1ea400d1928af3089279d1430695e958c2cb98a92b4c315dcdbbdae5d118606a3ccb1b63b1545574ae26264303c01f602f60c4335
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
    -DHDR_HISTOGRAM_BUILD_PROGRAMS=OFF
    -DHDR_HISTOGRAM_BUILD_SHARED=${BUILD_SHARED_LIBS}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME hdr_histogram CONFIG_PATH lib/cmake/hdr_histogram)
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
