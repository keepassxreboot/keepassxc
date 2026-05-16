find_package(PkgConfig REQUIRED)
pkg_check_modules(Xkbcommon REQUIRED IMPORTED_TARGET xkbcommon)

set(autotype_WAYLAND_SOURCES AutoTypeWayland.cpp)

add_library(keepassxc-autotype-wayland MODULE ${autotype_WAYLAND_SOURCES})
target_link_libraries(keepassxc-autotype-wayland keepassxc_gui Qt6::Core Qt6::DBus PkgConfig::Xkbcommon)
install(TARGETS keepassxc-autotype-wayland
        BUNDLE DESTINATION . COMPONENT Runtime
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
