include_directories(SYSTEM ${LIBVIRT_INCLUDE_DIR})

set(autotype_libvirt_SOURCES AutoTypeExtLibvirt.cpp)

add_library(keepassxc-autotype-ext-libvirt MODULE ${autotype_libvirt_SOURCES})
target_link_libraries(keepassxc-autotype-ext-libvirt keepassx_core Qt5::Core Qt5::Widgets ${LIBVIRT_LIBRARY})
install(TARGETS keepassxc-autotype-ext-libvirt
        BUNDLE DESTINATION . COMPONENT Runtime
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
