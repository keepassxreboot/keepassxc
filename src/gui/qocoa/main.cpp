#include <QtGui/QApplication>
#include "gallery.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    Gallery gallery;
    gallery.show();

    return application.exec();
}
