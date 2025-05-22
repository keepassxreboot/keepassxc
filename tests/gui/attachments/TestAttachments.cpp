#include <QtTest>

#include "TestPreviewEntryAttachmentsDialog.h"
#include "TestWidgetFactory.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    TestPreviewEntryAttachmentsDialog previewDialogTest{};
    TestAttachmentWidgetFactory factoryTest{};

    int result = 0;
    result |= QTest::qExec(&previewDialogTest, argc, argv);
    result |= QTest::qExec(&factoryTest, argc, argv);

    return result;
}