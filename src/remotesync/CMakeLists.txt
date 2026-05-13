include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Always-built foundation: provider abstraction + sync engine + command
# provider. Free of Qt6::Network so the command-based ("Script") sync flow
# that DatabaseWidget routes through RemoteSyncProvider::create("command", ...)
# remains compiled even when networking is disabled at build time.
set(remotesync_SOURCES
    RemoteSyncProvider.cpp
    CommandSyncProvider.cpp
    SyncEngine.cpp
)

set(remotesync_LIBS Qt6::Core Qt6::Widgets)

add_library(remotesync STATIC ${remotesync_SOURCES})
target_link_libraries(remotesync ${remotesync_LIBS})
