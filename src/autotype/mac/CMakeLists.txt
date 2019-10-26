set(autotype_mac_SOURCES AutoTypeMac.cpp)

add_library(keepassx-autotype-cocoa MODULE ${autotype_mac_SOURCES})
set_target_properties(keepassx-autotype-cocoa PROPERTIES LINK_FLAGS "-framework Foundation -framework AppKit")
target_link_libraries(keepassx-autotype-cocoa ${PROGNAME} Qt5::Core Qt5::Widgets)

if(WITH_APP_BUNDLE)
    add_custom_command(TARGET keepassx-autotype-cocoa
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libkeepassx-autotype-cocoa.so ${PLUGIN_INSTALL_DIR}
            COMMAND ${MACDEPLOYQT_EXE} ${PROGNAME}.app -executable=${PLUGIN_INSTALL_DIR}/libkeepassx-autotype-cocoa.so -no-plugins
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
            COMMENT "Deploying autotype plugin")
else()
    install(TARGETS keepassx-autotype-cocoa
            BUNDLE DESTINATION . COMPONENT Runtime
            LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
endif()
