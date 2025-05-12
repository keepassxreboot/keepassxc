#include "AttachmentWidgetFactory.h"
#include "ImageAttachmentsWidget.h"
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
        } else if (type == Tools::MimeType::Image) {
            return new ImageAttachmentsWidget(parent);
        }

        return new UnknownAttachmentTypeWidget(parent);
    }

} // namespace attachments
