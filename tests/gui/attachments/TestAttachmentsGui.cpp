#include <QtTest>

#include "TestAttachmentWidget.h"
#include "TestEditEntryAttachmentsDialog.h"
#include "TestImageAttachmentsView.h"
#include "TestImageAttachmentsWidget.h"
#include "TestPreviewEntryAttachmentsDialog.h"
#include "TestTextAttachmentsEditWidget.h"
#include "TestTextAttachmentsPreviewWidget.h"
#include "TestTextAttachmentsWidget.h"

#include <config-keepassx.h>
#include <gui/Application.h>

int main(int argc, char* argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    Application app(argc, argv);
    app.setApplicationName("KeePassXC");
    app.setApplicationVersion(KEEPASSXC_VERSION);
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    app.applyTheme();

    TestPreviewEntryAttachmentsDialog previewDialogTest{};
    TestEditEntryAttachmentsDialog editDialogTest{};
    TestTextAttachmentsWidget textAttachmentsWidget{};
    TestTextAttachmentsPreviewWidget textPreviewWidget{};
    TestTextAttachmentsEditWidget textEditWidget{};
    TestImageAttachmentsWidget imageWidget{};
    TestImageAttachmentsView imageView{};
    TestAttachmentsWidget attachmentWidget{};

    int result = 0;
    result |= QTest::qExec(&previewDialogTest, argc, argv);
    result |= QTest::qExec(&editDialogTest, argc, argv);
    result |= QTest::qExec(&textAttachmentsWidget, argc, argv);
    result |= QTest::qExec(&textPreviewWidget, argc, argv);
    result |= QTest::qExec(&textEditWidget, argc, argv);
    result |= QTest::qExec(&imageWidget, argc, argv);
    result |= QTest::qExec(&imageView, argc, argv);
    result |= QTest::qExec(&attachmentWidget, argc, argv);

    return result;
}
