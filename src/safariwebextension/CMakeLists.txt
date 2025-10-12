cmake_minimum_required(VERSION 3.10)

set(SOURCES
    SafariWebExtensionHandler.mm
)

# Specify the Swift source
add_executable(KeePassXCSafariWebExtension ${SOURCES})

target_compile_definitions(KeePassXCSafariWebExtension PRIVATE APP_GROUP_IDENTIFIER="${APPLE_TEAM_ID}.${APPLE_APP_IDENTIFIER}")
target_link_libraries(KeePassXCSafariWebExtension "-framework Foundation -framework SafariServices")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -e _NSExtensionMain")

configure_file(${CMAKE_SOURCE_DIR}/src/safariwebextension/safariwebextension.entitlements ${CMAKE_BINARY_DIR}/share/macosx/safariwebextension.entitlements)

set_target_properties(KeePassXCSafariWebExtension PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/src/safariwebextension/Info.plist
    BUNDLE_EXTENSION appex
    CPACK_BUNDLE_APPLE_ENTITLEMENTS "${CMAKE_BINARY_DIR}/share/macosx/safariwebextension.entitlements"
)

# Install the target as an app bundle
install(TARGETS KeePassXCSafariWebExtension
        BUNDLE DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)

# Compile browser extension and copy into Resources
set(WEB_EXTENSION_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/safariwebextension/keepassxc-browser")
set(BINARY_RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/KeePassXCSafariWebExtension.appex/Contents/Resources")

execute_process(
    COMMAND npm ci
    WORKING_DIRECTORY ${WEB_EXTENSION_SOURCE_DIR}
    RESULT_VARIABLE result
    ERROR_QUIET
    OUTPUT_QUIET
)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "npm ci failed with error code ${result}. Is the SafariWebExtension submodule cloned?")
endif()

file(MAKE_DIRECTORY ${BINARY_RESOURCES_DIR})

execute_process(
    COMMAND node build.js --skip-translations --safari-web-extension -o ${BINARY_RESOURCES_DIR}
    WORKING_DIRECTORY ${WEB_EXTENSION_SOURCE_DIR}
    RESULT_VARIABLE result
    ERROR_QUIET
    OUTPUT_QUIET
)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "Node.js build script failed with error code ${result}")
endif()

if(WITH_APP_BUNDLE)
    add_custom_command(TARGET KeePassXCSafariWebExtension
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PLUGIN_INSTALL_DIR}/KeePassXCSafariWebExtension.appex
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_BINARY_DIR}/KeePassXCSafariWebExtension.appex
                ${PLUGIN_INSTALL_DIR}/KeePassXCSafariWebExtension.appex
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
        COMMENT "Copying safariwebextension into plugins")
endif()
