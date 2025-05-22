#include "TextAttachmentsWidget.h"
#include "TextAttachmentsEditWidget.h"
#include "TextAttachmentsPreviewWidget.h"

#include "ui_TextAttachmentsWidget.h"

#include <QSplitter>
#include <QTextEdit>

TextAttachmentsWidget::TextAttachmentsWidget(QWidget* parent)
    : AbstractAttachmentWidget(parent)
    , m_ui(new Ui::TextAttachmentsWidget())
    , m_mode(attachments::OpenMode::ReadOnly)
{
    m_ui->setupUi(this);

    initWidget();
}

TextAttachmentsWidget::~TextAttachmentsWidget() = default;

void TextAttachmentsWidget::openAttachment(attachments::Attachment attachment, attachments::OpenMode mode)
{
    m_attachment = std::move(attachment);
    m_mode = mode;

    updateWidget();
}

attachments::Attachment TextAttachmentsWidget::getAttachment() const
{
    return m_attachment;
}

void TextAttachmentsWidget::updateWidget()
{
    if (m_mode == attachments::OpenMode::ReadOnly) {
        m_splitter->setSizes({0, 1});
        m_editWidget->hide();
    } else {
        m_splitter->setSizes({1, 0});
        m_editWidget->show();
    }

    m_editWidget->openAttachment(m_attachment, m_mode);
    m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
}

void TextAttachmentsWidget::initWidget()
{
    m_splitter = new QSplitter(this);
    auto editWidget = new TextAttachmentsEditWidget(this);
    m_editWidget = editWidget;

    m_previewWidget = new TextAttachmentsPreviewWidget(this);

    connect(editWidget, &TextAttachmentsEditWidget::textChanged, [this]() {
        m_attachment = m_editWidget->getAttachment();
        m_previewWidget->openAttachment(m_attachment, attachments::OpenMode::ReadOnly);
    });

    connect(editWidget, &TextAttachmentsEditWidget::previewButtonClicked, [this]() {
        const auto sizes = m_splitter->sizes();

        const auto previewSize = sizes.value(1, 0) > 0 ? 0 : 1;
        m_splitter->setSizes({1, previewSize});
    });

    m_splitter->addWidget(m_editWidget);

    // Prevent collapsing of the edit widget
    m_splitter->setCollapsible(0, false);

    m_splitter->addWidget(m_previewWidget);

    m_ui->verticalLayout->addWidget(m_splitter);

    updateWidget();
}
