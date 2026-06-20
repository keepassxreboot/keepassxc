# KeePassXC Core Library
# Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>

# Core source files
set(CORE_SOURCES
    Alloc.cpp
    AutoTypeAssociations.cpp
    AutoTypeMatch.cpp
    AutoType.cpp
    Bootstrap.cpp
    Clock.cpp
    CsvParser.cpp
    CustomData.cpp
    Database.cpp
    DatabaseIcons.cpp
    DatabaseManager.cpp
    DatabaseWidget.cpp
    Entry.cpp
    EntryAttributes.cpp
    EntrySearcher.cpp
    EntryAttachments.cpp
    EntryHistory.cpp
    Global.cpp
    Group.cpp
    Merger.cpp
    Metadata.cpp
    PasswordGenerator.cpp
    PasswordHealth.cpp
    qrcode/UriParser.cpp
    RemoteDatabaseManager.cpp
    Resources.cpp
    ShortcutSettings.cpp
    Tools.cpp
    Translator.cpp
    Uuid.cpp
    qrcode/QrCode.cpp
)

# Core header files
set(CORE_HEADERS
    Alloc.h
    AutoTypeAssociations.h
    AutoTypeMatch.h
    AutoType.h
    Bootstrap.h
    Clock.h
    CsvParser.h
    CustomData.h
    Database.h
    DatabaseIcons.h
    DatabaseManager.h
    DatabaseWidget.h
    Entry.h
    EntryAttributes.h
    EntrySearcher.h
    EntryAttachments.h
    EntryHistory.h
    Global.h
    Group.h
    Merger.h
    Metadata.h
    PasswordGenerator.h
    PasswordHealth.h
    qrcode/UriParser.h
    RemoteDatabaseManager.h
    Resources.h
    SignalT.h
    ShortcutSettings.h
    Tools.h
    Translator.h
    Uuid.h
    qrcode/QrCode.h
)

# Add MinGW compatibility files
if(WIN32 AND MINGW)
    list(APPEND CORE_SOURCES
        compat/mingw_compat.cpp
    )
    list(APPEND CORE_HEADERS
        compat/mingw_compat.h
    )
endif()

# Create the core library
add_library(kpxc_core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

# Set include directories
target_include_directories(kpxc_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link libraries
target_link_libraries(kpxc_core PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
)

# Optional libraries
if(WITH_XC_NETWORKING)
    target_link_libraries(kpxc_core PUBLIC
        ${CURL_LIBRARIES}
    )
    target_include_directories(kpxc_core PUBLIC
        ${CURL_INCLUDE_DIRS}
    )
endif()

# Platform specific configurations
if(WIN32)
    target_link_libraries(kpxc_core PUBLIC
        winmm
        ws2_32
        shlwapi
        crypt32
        ole32
        uuid
    )
    
    if(MINGW)
        # MinGW specific compile definitions
        target_compile_definitions(kpxc_core PRIVATE
            MINGW_HAS_SECURE_API=1
            __USE_MINGW_ANSI_STDIO=1
        )
    endif()
endif()

if(APPLE)
    target_link_libraries(kpxc_core PUBLIC
        ${CARBON_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${SECURITY_LIBRARY}
        ${COCOA_LIBRARY}
    )
endif()

# Set properties
set_target_properties(kpxc_core PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)