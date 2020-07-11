#include "EntryAttachmentsWidget.h"
#include "ui_EntryAttachmentsWidget.h"

#include <QDesktopServices>
#include <QDir>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QProcessEnvironment>
#include <QTemporaryFile>

#include "EntryAttachmentsModel.h"
#include "config-keepassx.h"
#include "core/Config.h"
#include "core/EntryAttachments.h"
#include "core/Tools.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"

EntryAttachmentsWidget::EntryAttachmentsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EntryAttachmentsWidget)
    , m_entryAttachments(new EntryAttachments(this))
    , m_attachmentsModel(new EntryAttachmentsModel(this))
    , m_readOnly(false)
    , m_buttonsVisible(true)
{
    m_ui->setupUi(this);

    m_ui->attachmentsView->setAcceptDrops(false);
    m_ui->attachmentsView->viewport()->setAcceptDrops(true);
    m_ui->attachmentsView->viewport()->installEventFilter(this);

    m_attachmentsModel->setEntryAttachments(m_entryAttachments);
    m_ui->attachmentsView->setModel(m_attachmentsModel);
    m_ui->attachmentsView->verticalHeader()->hide();
    m_ui->attachmentsView->horizontalHeader()->setStretchLastSection(true);
    m_ui->attachmentsView->horizontalHeader()->resizeSection(EntryAttachmentsModel::NameColumn, 400);
    m_ui->attachmentsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->attachmentsView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, SIGNAL(buttonsVisibleChanged(bool)), this, SLOT(updateButtonsVisible()));
    connect(this, SIGNAL(readOnlyChanged(bool)), SLOT(updateButtonsEnabled()));
    connect(m_attachmentsModel, SIGNAL(modelReset()), SLOT(updateButtonsEnabled()));

    // clang-format off
    connect(m_ui->attachmentsView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(updateButtonsEnabled()));
    // clang-format on
    connect(this, SIGNAL(readOnlyChanged(bool)), m_attachmentsModel, SLOT(setReadOnly(bool)));

    connect(m_ui->attachmentsView, SIGNAL(doubleClicked(QModelIndex)), SLOT(openAttachment(QModelIndex)));
    connect(m_ui->saveAttachmentButton, SIGNAL(clicked()), SLOT(saveSelectedAttachments()));
    connect(m_ui->openAttachmentButton, SIGNAL(clicked()), SLOT(openSelectedAttachments()));
    connect(m_ui->addAttachmentButton, SIGNAL(clicked()), SLOT(insertAttachments()));
    connect(m_ui->removeAttachmentButton, SIGNAL(clicked()), SLOT(removeSelectedAttachments()));
    connect(m_ui->renameAttachmentButton, SIGNAL(clicked()), SLOT(renameSelectedAttachments()));

    updateButtonsVisible();
    updateButtonsEnabled();
}

EntryAttachmentsWidget::~EntryAttachmentsWidget()
{
}

const EntryAttachments* EntryAttachmentsWidget::entryAttachments() const
{
    return m_entryAttachments;
}

bool EntryAttachmentsWidget::isReadOnly() const
{
    return m_readOnly;
}

bool EntryAttachmentsWidget::isButtonsVisible() const
{
    return m_buttonsVisible;
}

void EntryAttachmentsWidget::setEntryAttachments(const EntryAttachments* attachments)
{
    Q_ASSERT(attachments != nullptr);
    m_entryAttachments->copyDataFrom(attachments);
}

void EntryAttachmentsWidget::clearAttachments()
{
    m_entryAttachments->clear();
}

void EntryAttachmentsWidget::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly) {
        return;
    }

    m_readOnly = readOnly;
    emit readOnlyChanged(m_readOnly);
}

void EntryAttachmentsWidget::setButtonsVisible(bool buttonsVisible)
{
    if (m_buttonsVisible == buttonsVisible) {
        return;
    }

    m_buttonsVisible = buttonsVisible;
    emit buttonsVisibleChanged(m_buttonsVisible);
}

QByteArray EntryAttachmentsWidget::getAttachment(const QString& name)
{
    return m_entryAttachments->value(name);
}

void EntryAttachmentsWidget::setAttachment(const QString& name, const QByteArray& value)
{
    m_entryAttachments->set(name, value);
}

void EntryAttachmentsWidget::removeAttachment(const QString& name)
{
    if (!isReadOnly() && m_entryAttachments->hasKey(name)) {
        m_entryAttachments->remove(name);
    }
}

void EntryAttachmentsWidget::insertAttachments()
{
    Q_ASSERT(!isReadOnly());
    if (isReadOnly()) {
        return;
    }

    QString defaultDirPath = config()->get(Config::LastAttachmentDir).toString();
    const bool dirExists = !defaultDirPath.isEmpty() && QDir(defaultDirPath).exists();
    if (!dirExists) {
        defaultDirPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
    }

    const auto filenames = fileDialog()->getOpenFileNames(this, tr("Select files"), defaultDirPath);
    if (filenames.isEmpty()) {
        return;
    }
    const auto confirmedFileNames = confirmLargeAttachments(filenames);
    if (confirmedFileNames.isEmpty()) {
        return;
    }
    config()->set(Config::LastAttachmentDir, QFileInfo(filenames.first()).absolutePath());
    QString errorMessage;
    if (!insertAttachments(confirmedFileNames, errorMessage)) {
        errorOccurred(errorMessage);
    }
    emit widgetUpdated();
}

void EntryAttachmentsWidget::removeSelectedAttachments()
{
    Q_ASSERT(!isReadOnly());
    if (isReadOnly()) {
        return;
    }

    const QModelIndexList indexes = m_ui->attachmentsView->selectionModel()->selectedRows(0);
    if (indexes.isEmpty()) {
        return;
    }

    auto result = MessageBox::question(this,
                                       tr("Confirm remove"),
                                       tr("Are you sure you want to remove %n attachment(s)?", "", indexes.count()),
                                       MessageBox::Remove | MessageBox::Cancel,
                                       MessageBox::Cancel);

    if (result == MessageBox::Remove) {
        QStringList keys;
        for (const QModelIndex& index : indexes) {
            keys.append(m_attachmentsModel->keyByIndex(index));
        }
        m_entryAttachments->remove(keys);
        emit widgetUpdated();
    }
}

void EntryAttachmentsWidget::renameSelectedAttachments()
{
    m_ui->attachmentsView->edit(m_ui->attachmentsView->selectionModel()->selectedIndexes().first());
}

void EntryAttachmentsWidget::saveSelectedAttachments()
{
    const QModelIndexList indexes = m_ui->attachmentsView->selectionModel()->selectedRows(0);
    if (indexes.isEmpty()) {
        return;
    }

    QString defaultDirPath = config()->get(Config::LastAttachmentDir).toString();
    const bool dirExists = !defaultDirPath.isEmpty() && QDir(defaultDirPath).exists();
    if (!dirExists) {
        defaultDirPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    const QString saveDirPath = fileDialog()->getExistingDirectory(this, tr("Save attachments"), defaultDirPath);
    if (saveDirPath.isEmpty()) {
        return;
    }

    QDir saveDir(saveDirPath);
    if (!saveDir.exists()) {
        if (saveDir.mkpath(saveDir.absolutePath())) {
            errorOccurred(tr("Unable to create directory:\n%1").arg(saveDir.absolutePath()));
            return;
        }
    }
    config()->set(Config::LastAttachmentDir, QFileInfo(saveDir.absolutePath()).absolutePath());

    QStringList errors;
    for (const QModelIndex& index : indexes) {
        const QString filename = m_attachmentsModel->keyByIndex(index);
        const QString attachmentPath = saveDir.absoluteFilePath(filename);

        if (QFileInfo::exists(attachmentPath)) {

            MessageBox::Buttons buttons = MessageBox::Overwrite | MessageBox::Cancel;
            if (indexes.length() > 1) {
                buttons |= MessageBox::Skip;
            }

            const QString questionText(
                tr("Are you sure you want to overwrite the existing file \"%1\" with the attachment?"));

            auto result = MessageBox::question(
                this, tr("Confirm overwrite"), questionText.arg(filename), buttons, MessageBox::Cancel);

            if (result == MessageBox::Skip) {
                continue;
            } else if (result == MessageBox::Cancel) {
                return;
            }
        }

        QFile file(attachmentPath);
        const QByteArray attachmentData = m_entryAttachments->value(filename);
        const bool saveOk = file.open(QIODevice::WriteOnly) && file.write(attachmentData) == attachmentData.size();
        if (!saveOk) {
            errors.append(QString("%1 - %2").arg(filename, file.errorString()));
        }
    }

    if (!errors.isEmpty()) {
        errorOccurred(tr("Unable to save attachments:\n%1").arg(errors.join('\n')));
    }
}

void EntryAttachmentsWidget::openAttachment(const QModelIndex& index)
{
    Q_ASSERT(index.isValid());
    if (!index.isValid()) {
        return;
    }

    QString errorMessage;
    if (!openAttachment(index, errorMessage)) {
        errorOccurred(tr("Unable to open attachment:\n%1").arg(errorMessage));
    }
}

void EntryAttachmentsWidget::openSelectedAttachments()
{
    const QModelIndexList indexes = m_ui->attachmentsView->selectionModel()->selectedRows(0);
    if (indexes.isEmpty()) {
        return;
    }

    QStringList errors;
    for (const QModelIndex& index : indexes) {
        QString errorMessage;
        if (!openAttachment(index, errorMessage)) {
            const QString filename = m_attachmentsModel->keyByIndex(index);
            errors.append(QString("%1 - %2").arg(filename, errorMessage));
        };
    }

    if (!errors.isEmpty()) {
        errorOccurred(tr("Unable to open attachments:\n%1").arg(errors.join('\n')));
    }
}

void EntryAttachmentsWidget::updateButtonsEnabled()
{
    const bool hasSelection = m_ui->attachmentsView->selectionModel()->hasSelection();

    m_ui->addAttachmentButton->setEnabled(!m_readOnly);
    m_ui->removeAttachmentButton->setEnabled(hasSelection && !m_readOnly);
    m_ui->renameAttachmentButton->setEnabled(hasSelection && !m_readOnly);

    m_ui->saveAttachmentButton->setEnabled(hasSelection);
    m_ui->openAttachmentButton->setEnabled(hasSelection);
}

void EntryAttachmentsWidget::updateButtonsVisible()
{
    m_ui->addAttachmentButton->setVisible(m_buttonsVisible && !m_readOnly);
    m_ui->removeAttachmentButton->setVisible(m_buttonsVisible && !m_readOnly);
}

bool EntryAttachmentsWidget::insertAttachments(const QStringList& filenames, QString& errorMessage)
{
    Q_ASSERT(!isReadOnly());
    if (isReadOnly()) {
        return false;
    }

    QStringList errors;
    for (const QString& filename : filenames) {
        QByteArray data;
        QFile file(filename);
        const QFileInfo fInfo(filename);
        const bool readOk = file.open(QIODevice::ReadOnly) && Tools::readAllFromDevice(&file, data);
        if (readOk) {
            m_entryAttachments->set(fInfo.fileName(), data);
        } else {
            errors.append(QString("%1 - %2").arg(fInfo.fileName(), file.errorString()));
        }
    }

    if (!errors.isEmpty()) {
        errorMessage = tr("Unable to open file(s):\n%1", "", errors.size()).arg(errors.join('\n'));
    }

    return errors.isEmpty();
}

bool EntryAttachmentsWidget::openAttachment(const QModelIndex& index, QString& errorMessage)
{
    const QString filename = m_attachmentsModel->keyByIndex(index);
    const QByteArray attachmentData = m_entryAttachments->value(filename);

    // tmp file will be removed once the database (or the application) has been closed
#ifdef KEEPASSXC_DIST_SNAP
    const QString tmpFileTemplate =
        QString("%1/XXXXXX.%2").arg(QProcessEnvironment::systemEnvironment().value("SNAP_USER_DATA"), filename);
#else
    const QString tmpFileTemplate = QDir::temp().absoluteFilePath(QString("XXXXXX.").append(filename));
#endif

    QScopedPointer<QTemporaryFile> tmpFile(new QTemporaryFile(tmpFileTemplate, this));

    const bool saveOk = tmpFile->open() && tmpFile->write(attachmentData) == attachmentData.size() && tmpFile->flush();
    if (!saveOk) {
        errorMessage = QString("%1 - %2").arg(filename, tmpFile->errorString());
        return false;
    }

    tmpFile->close();
    const bool openOk = QDesktopServices::openUrl(QUrl::fromLocalFile(tmpFile->fileName()));
    if (!openOk) {
        errorMessage = QString("Can't open file \"%1\"").arg(filename);
        return false;
    }

    // take ownership of the tmpFile pointer
    tmpFile.take();
    return true;
}

QStringList EntryAttachmentsWidget::confirmLargeAttachments(const QStringList& filenames)
{
    const QString confirmation(tr("%1 is a big file (%2 MB).\nYour database may get very large and reduce "
                                  "performance.\n\nAre you sure to add this file?"));
    QStringList confirmedFileNames;
    for (const auto& file : filenames) {
        QFileInfo fileInfo(file);
        double size = fileInfo.size() / (1024.0 * 1024.0);
        // Ask for confirmation before adding files over 5 MB in size
        if (size > 5.0) {
            auto fileName = fileInfo.fileName();
            auto result = MessageBox::question(this,
                                               tr("Confirm Attachment"),
                                               confirmation.arg(fileName, QString::number(size, 'f', 1)),
                                               MessageBox::Yes | MessageBox::No,
                                               MessageBox::No);
            if (result == MessageBox::Yes) {
                confirmedFileNames << file;
            }
        } else {
            confirmedFileNames << file;
        }
    }

    return confirmedFileNames;
}

bool EntryAttachmentsWidget::eventFilter(QObject* watched, QEvent* e)
{
    if (watched == m_ui->attachmentsView->viewport() && !isReadOnly()) {
        const QEvent::Type eventType = e->type();
        if (eventType == QEvent::DragEnter || eventType == QEvent::DragMove) {
            QDropEvent* dropEv = static_cast<QDropEvent*>(e);
            const QMimeData* mimeData = dropEv->mimeData();
            if (mimeData->hasUrls()) {
                dropEv->acceptProposedAction();
                return true;
            }
        } else if (eventType == QEvent::Drop) {
            QDropEvent* dropEv = static_cast<QDropEvent*>(e);
            const QMimeData* mimeData = dropEv->mimeData();
            if (mimeData->hasUrls()) {
                dropEv->acceptProposedAction();
                QStringList filenames;
                const QList<QUrl> urls = mimeData->urls();
                for (const QUrl& url : urls) {
                    const QFileInfo fInfo(url.toLocalFile());
                    if (fInfo.isFile()) {
                        filenames.append(fInfo.absoluteFilePath());
                    }
                }

                QString errorMessage;
                if (!insertAttachments(filenames, errorMessage)) {
                    errorOccurred(errorMessage);
                }

                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, e);
}
