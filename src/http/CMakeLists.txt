if(WITH_XC_HTTP)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    
    set(keepasshttp_SOURCES
        AccessControlDialog.cpp
        EntryConfig.cpp
        HttpPasswordGeneratorWidget.cpp
        HttpSettings.cpp
        OptionDialog.cpp
        Protocol.cpp
        Server.cpp
        Service.cpp
    )

    add_library(keepasshttp STATIC ${keepasshttp_SOURCES})
    target_link_libraries(keepasshttp qhttp Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network)
endif()
