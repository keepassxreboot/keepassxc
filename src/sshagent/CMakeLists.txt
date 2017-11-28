if(WITH_XC_SSHAGENT)
    set(sshagent_SOURCES
        bcrypt_pbkdf.cpp
        blowfish.c
        AgentSettingsPage.cpp
        AgentSettingsWidget.cpp
        BinaryStream.cpp
        KeeAgentSettings.cpp
        OpenSSHKey.cpp
        SSHAgent.cpp
    )

    add_library(sshagent STATIC ${sshagent_SOURCES})
    target_link_libraries(sshagent Qt5::Core Qt5::Widgets Qt5::Network ${GCRYPT_LIBRARIES})
endif()
