if(WITH_XC_KEESHARE)
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
	ShareObserver.cpp
	Signature.cpp
    )

    add_library(keeshare STATIC ${keeshare_SOURCES})
    target_link_libraries(keeshare Qt5::Core Qt5::Widgets ${GCRYPT_LIBRARIES} ${QUAZIP_LIBRARIES} ${crypto_ssh_LIB})
endif()
