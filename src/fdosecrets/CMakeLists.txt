if(WITH_XC_FDOSECRETS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    add_library(fdosecrets STATIC
        # app settings page
        FdoSecretsPlugin.cpp
        widgets/SettingsWidgetFdoSecrets.cpp

        # per database settings page
        DatabaseSettingsPageFdoSecrets.cpp
        widgets/DatabaseSettingsWidgetFdoSecrets.cpp

        # setting storage
        FdoSecretsSettings.cpp

        # gcrypt MPI wrapper
        GcryptMPI.cpp

        # dbus objects
        objects/DBusObject.cpp
        objects/Service.cpp
        objects/Session.cpp
        objects/SessionCipher.cpp
        objects/Collection.cpp
        objects/Item.cpp
        objects/Prompt.cpp
        objects/adaptors/ServiceAdaptor.cpp
        objects/adaptors/SessionAdaptor.cpp
        objects/adaptors/CollectionAdaptor.cpp
        objects/adaptors/ItemAdaptor.cpp
        objects/adaptors/PromptAdaptor.cpp
        objects/DBusReturn.cpp
        objects/DBusTypes.cpp
    )
    target_link_libraries(fdosecrets Qt5::Core Qt5::Widgets Qt5::DBus ${GCRYPT_LIBRARIES})
endif()
