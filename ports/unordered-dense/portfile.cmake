vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO redpanda-data/unordered_dense
    REF 9338f301522a965309ecec58ce61f54a52fb5c22
    SHA512 dc5f2a20c54e747afefb44aa71647ec15919339dffd1d201e852ecb1fd5a62b0f0eaaa465d64fc30f5eedc990a61dfaaa151169b89f30efe7fce399ac51dd367
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH ${SOURCE_PATH}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME unordered_dense
    CONFIG_PATH lib/cmake/unordered_dense)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
