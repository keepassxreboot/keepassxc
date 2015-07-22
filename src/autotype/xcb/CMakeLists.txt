include_directories(SYSTEM ${X11_X11_INCLUDE_PATH})

set(autotype_XCB_SOURCES
    AutoTypeXCB.cpp
)

add_library(keepassx-autotype-xcb MODULE ${autotype_XCB_SOURCES})
target_link_libraries(keepassx-autotype-xcb Qt5::Core Qt5::Widgets Qt5::X11Extras ${X11_X11_LIB} ${X11_Xi_LIB} ${X11_XTest_LIB})
install(TARGETS keepassx-autotype-xcb
        BUNDLE DESTINATION . COMPONENT Runtime
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
