include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Always-built foundation: provider abstraction + sync engine + command
# provider. Free of Qt6::Network so the command-based ("Script") sync flow
# that DatabaseWidget routes through RemoteSyncProvider::create("command", ...)
# remains compiled even when networking is disabled at build time.
set(remotesync_SOURCES
    RemoteSyncParams.cpp
    RemoteSyncProvider.cpp
    CommandSyncProvider.cpp
    SyncEngine.cpp
)

set(remotesync_LIBS Qt6::Core Qt6::Widgets)

# Cloud providers require Qt6::Network. They drop out when the project is
# built with -DKPXC_FEATURE_NETWORK=OFF, the same flag that already governs
# update check and other network-using subsystems.
if(KPXC_FEATURE_NETWORK)
    list(APPEND remotesync_SOURCES
        HttpRetryHelper.cpp
        DropboxSyncProvider.cpp
        DropboxLoginFlow.cpp
        NextcloudSyncProvider.cpp
        NextcloudLoginFlow.cpp
        OAuthHttpServer.cpp
    )
    list(APPEND remotesync_LIBS Qt6::Network)
endif()

add_library(remotesync STATIC ${remotesync_SOURCES})
target_link_libraries(remotesync ${remotesync_LIBS})
