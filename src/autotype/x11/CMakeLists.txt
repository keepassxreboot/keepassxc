include_directories(SYSTEM ${X11_X11_INCLUDE_PATH})

set(autotype_X11_SOURCES
    AutoTypeX11.cpp
)

add_library(keepassx-autotype-x11 MODULE ${autotype_X11_SOURCES})
target_link_libraries(keepassx-autotype-x11 Qt5::Core Qt5::Widgets Qt5::X11Extras ${X11_X11_LIB} ${X11_Xi_LIB} ${X11_XTest_LIB})
install(TARGETS keepassx-autotype-x11
        BUNDLE DESTINATION . COMPONENT Runtime
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
