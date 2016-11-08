set(autotype_mac_SOURCES
    AutoTypeMac.cpp
)

set(autotype_mac_mm_SOURCES
    AppKitImpl.mm
)

add_library(keepassx-autotype-cocoa MODULE ${autotype_mac_SOURCES} ${autotype_mac_mm_SOURCES})
set_target_properties(keepassx-autotype-cocoa PROPERTIES LINK_FLAGS "-framework Foundation -framework AppKit -framework Carbon")
target_link_libraries(keepassx-autotype-cocoa ${PROGNAME} Qt5::Core Qt5::Widgets)
if(NOT DEFINED QT_BINARY_DIR)
  set(QT_BINARY_DIR "/usr/local/opt/qt5/bin" CACHE PATH "QT binary folder")
endif()
add_custom_command(TARGET keepassx-autotype-cocoa
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libkeepassx-autotype-cocoa.so ${PLUGIN_INSTALL_DIR}
                   COMMAND ${QT_BINARY_DIR}/macdeployqt ${PROGNAME}.app -executable=${PLUGIN_INSTALL_DIR}/libkeepassx-autotype-cocoa.so -no-plugins
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src
COMMENT "Deploying autotype plugin")
