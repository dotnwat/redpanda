vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO bytecodealliance/wasmtime
    REF 9e1084ffac08b1bf9c82de40c0efc1baff14b9ad
    SHA512 70b150bf0b9aca420c0cb18aba1fdffaafe36da6feb6910c312b0f76d8b27c06a88a76db1026b0d584d12d43028367006f0518479ddbdc3d65a95075023d139d
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/crates/c-api"
    OPTIONS
    -DWASMTIME_USER_CARGO_BUILD_OPTIONS=--no-default-features\;--features=async\;--features=addr2line\;--features=wat
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
