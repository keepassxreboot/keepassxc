include_directories(SYSTEM ${LIBVIRT_INCLUDE_DIR})

set(autotype_libvirt_SOURCES AutoTypeExtLibvirt.cpp)

add_library(keepassx-autotype-ext-libvirt MODULE ${autotype_libvirt_SOURCES})
target_link_libraries(keepassx-autotype-ext-libvirt keepassx_core Qt5::Core Qt5::Widgets ${LIBVIRT_LIBRARY})
install(TARGETS keepassx-autotype-ext-libvirt
        BUNDLE DESTINATION . COMPONENT Runtime
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
