# Draft vcpkg portfile for novada-cpp.
#
# To publish to a registry, replace REF/SHA512 with the tagged release commit
# and its archive hash (run `vcpkg install novada` once to be told the correct
# SHA512, or use `vcpkg hash <file>`).
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO NovadaLabs/novada-cpp
    REF "v${VERSION}"
    SHA512 0  # placeholder: fill in the real archive SHA512 before publishing
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DNOVADA_BUILD_TESTS=OFF
        -DNOVADA_BUILD_EXAMPLES=OFF
        -DNOVADA_INSTALL=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME novada CONFIG_PATH lib/cmake/novada)

# Header/library layout: remove the duplicated include tree from the debug
# build and drop empty directories.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
