set(autotype_test_SOURCES
    AutoTypeTest.cpp
)

set(autotype_test_MOC
    AutoTypeTest.h
)

qt4_wrap_cpp(autotype_test_SOURCES ${autotype_test_MOC})

add_library(keepassx-autotype-test MODULE ${autotype_test_SOURCES})
target_link_libraries(keepassx-autotype-test testautotype ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
