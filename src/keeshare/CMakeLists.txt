if(WITH_XC_KEESHARE)
    set(WITH_XC_KEESHARE_INSECURE ON PARENT_SCOPE)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    set(keeshare_SOURCES
        SettingsPageKeeShare.cpp
        SettingsWidgetKeeShare.cpp
        DatabaseSettingsPageKeeShare.cpp
        DatabaseSettingsWidgetKeeShare.cpp
        group/EditGroupWidgetKeeShare.cpp
        group/EditGroupPageKeeShare.cpp
        KeeShare.cpp
        KeeShareSettings.cpp
        ShareImport.cpp
        ShareExport.cpp
        ShareObserver.cpp
        Signature.cpp
    )

    add_library(keeshare STATIC ${keeshare_SOURCES})
    target_link_libraries(keeshare PUBLIC Qt5::Core Qt5::Widgets ${GCRYPT_LIBRARIES} ${crypto_ssh_LIB})

    # Try to find libquazip5, if found, enable secure sharing
    find_package(QuaZip)
    if(QUAZIP_FOUND)
        set(WITH_XC_KEESHARE_SECURE ON PARENT_SCOPE)
        target_include_directories(keeshare SYSTEM PRIVATE ${QUAZIP_INCLUDE_DIR})
        target_link_libraries(keeshare PRIVATE ${QUAZIP_LIBRARIES})
    else()
        set(WITH_XC_KEESHARE_SECURE OFF PARENT_SCOPE)
        message(STATUS "KeeShare: Secure container support is DISABLED; quazip library not found")
    endif()
else(WITH_XC_KEESHARE)
    set(WITH_XC_KEESHARE_INSECURE OFF PARENT_SCOPE)
    set(WITH_XC_KEESHARE_SECURE OFF PARENT_SCOPE)
endif(WITH_XC_KEESHARE)
