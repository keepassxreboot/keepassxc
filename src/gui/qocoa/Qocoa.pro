SOURCES += main.cpp\
           gallery.cpp \

HEADERS += gallery.h \
           qocoa_mac.h \
           qsearchfield.h \
           qbutton.h \
           qprogressindicatorspinning.h \

mac {
    OBJECTIVE_SOURCES += qsearchfield_mac.mm qbutton_mac.mm qprogressindicatorspinning_mac.mm
    LIBS += -framework Foundation -framework Appkit
    QMAKE_CFLAGS += -mmacosx-version-min=10.6
} else {
    SOURCES += qsearchfield_nonmac.cpp qbutton_nonmac.cpp qprogressindicatorspinning_nonmac.cpp
    RESOURCES += qsearchfield_nonmac.qrc qprogressindicatorspinning_nonmac.qrc
}
