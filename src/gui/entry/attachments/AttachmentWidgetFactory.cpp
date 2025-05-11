#include "AttachmentWidgetFactory.h"
#include "TextAttachmentsWidget.h"
#include "UnknownAttachmentTypeWidget.h"

#include <QPointer>

namespace attachments
{
    QPointer<AbstractAttachmentWidget> AttachmentsWidgetFactory::createAttachmentWidget(Tools::MimeType type,
                                                                                        QWidget* parent)
    {
        if (type == Tools::MimeType::PlainText) {
            return new TextAttachmentsWidget(parent);
        }

        return new UnknownAttachmentTypeWidget(parent);
    }

} // namespace attachments
