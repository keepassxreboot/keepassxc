vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO P-H-C/phc-winner-argon2
    REF 20190702
    SHA512 0a4cb89e8e63399f7df069e2862ccd05308b7652bf4ab74372842f66bcc60776399e0eaf979a7b7e31436b5e6913fe5b0a6949549d8c82ebd06e0629b106e85f    
    HEAD_REF master
)

configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt ${SOURCE_PATH}/CMakeLists.txt COPYONLY)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        ${FEATURE_OPTIONS}
    OPTIONS_DEBUG
        -DDISABLE_INSTALL_HEADERS=ON
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
