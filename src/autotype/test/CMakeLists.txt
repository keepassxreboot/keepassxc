set(autotype_test_SOURCES AutoTypeTest.cpp)

add_library(keepassxc-autotype-test MODULE ${autotype_test_SOURCES})
target_link_libraries(keepassxc-autotype-test keepassx_core ${autotype_LIB} Qt5::Core Qt5::Widgets)
