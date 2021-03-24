set(autotype_ext_test_SOURCES AutoTypeExtTest.cpp)

add_library(keepassxc-autotype-ext-test MODULE ${autotype_ext_test_SOURCES})
target_link_libraries(keepassxc-autotype-ext-test keepassx_core ${autotype_LIB} Qt5::Core Qt5::Widgets)
