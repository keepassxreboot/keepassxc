set(autotype_mac_SOURCES AutoTypeMac.cpp)

add_library(keepassxc-autotype-cocoa MODULE ${autotype_mac_SOURCES})
set_target_properties(keepassxc-autotype-cocoa PROPERTIES LINK_FLAGS "-framework Foundation -framework AppKit -framework Carbon")
target_link_libraries(keepassxc-autotype-cocoa ${PROGNAME} Qt5::Core Qt5::Widgets)

if(WITH_APP_BUNDLE)
    add_custom_command(TARGET keepassxc-autotype-cocoa
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libkeepassxc-autotype-cocoa.so ${PLUGIN_INSTALL_DIR}/libkeepassxc-autotype-cocoa.so
            COMMAND ${MACDEPLOYQT_EXE} ${PROGNAME}.app -executable=${PLUGIN_INSTALL_DIR}/libkeepassxc-autotype-cocoa.so -no-plugins 2> /dev/null
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
            COMMENT "Deploying autotype plugin")
else()
    install(TARGETS keepassxc-autotype-cocoa
            BUNDLE DESTINATION . COMPONENT Runtime
            LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
endif()
