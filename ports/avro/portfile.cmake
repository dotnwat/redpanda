vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO redpanda-data/avro
    #REF d9f4cee17241f70554c6bcd0ba914a90b67b05cc
    REF e6bfdcddd814839eea917b9477ffa46e2823b00d
    #SHA512 634e86fcf72c2c402562850130caa3ea7f93c9ee8f7be6693cfa73be2871ccc957de1e8fb255815365f2a986dda60587ea494a746633b3c0162f4a143fc8a691
    SHA512 1ef1cfbf70fb8c9b605bf1db9626c586065c5e52dc7113cb8f5a1680e1ac38527b3231d6569d6f10d6e8f959e956610e4617d7478d0b05df98e0b5a6fad1572a
    HEAD_REF release-1.11.1-redpanda
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/redpanda_build"
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
