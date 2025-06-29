/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "EntryAttachmentsWidget.h"

#include "EditEntryAttachmentsDialog.h"
#include "EntryAttachmentsModel.h"
#include "PreviewEntryAttachmentsDialog.h"
#include "ui_EntryAttachmentsWidget.h"

#include <QDebug>
#include <QDropEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QStandardPaths>
#include <QTemporaryFile>

#include "core/EntryAttachments.h"
#include "core/Tools.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"

namespace
{
    constexpr const char* Suffix = ".txt";

    QString generateUniqueName(const QString& name, const QStringList& existingNames)
    {
        uint64_t i = 0;
        QString newName = QStringLiteral("%1%2").arg(name).arg(Suffix);
        while (existingNames.contains(newName)) {
            newName = QStringLiteral("%1_%2%3").arg(name).arg(++i).arg(Suffix);
        }
        return newName;
    }

} // namespace

EntryAttachmentsWidget::EntryAttachmentsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EntryAttachmentsWidget)
    , m_entryAttachments(nullptr)
    , m_attachmentsModel(new EntryAttachmentsModel(this))
    , m_readOnly(false)
    , m_buttonsVisible(true)
{
    m_ui->setupUi(this);

    m_ui->attachmentsView->setAcceptDrops(false);
    m_ui->attachmentsView->viewport()->setAcceptDrops(true);
    m_ui->attachmentsView->viewport()->installEventFilter(this);

    m_ui->attachmentsView->setModel(m_attachmentsModel);
    m_ui->attachmentsView->horizontalHeader()->setMinimumSectionSize(70);
    m_ui->attachmentsView->horizontalHeader()->setSectionResizeMode(EntryAttachmentsModel::NameColumn,
                                                                    QHeaderView::Stretch);
    m_ui->attachmentsView->horizontalHeader()->setSectionResizeMode(EntryAttachmentsModel::SizeColumn,
                                                                    QHeaderView::ResizeToContents);
    m_ui->attachmentsView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    connect(this, SIGNAL(buttonsVisibleChanged(bool)), this, SLOT(updateButtonsVisible()));
    connect(this, SIGNAL(readOnlyChanged(bool)), SLOT(updateButtonsEnabled()));
    connect(m_attachmentsModel, SIGNAL(modelReset()), SLOT(updateButtonsEnabled()));

    // clang-format off
    connect(m_ui->attachmentsView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(updateButtonsEnabled()));
    // clang-format on
    connect(this, SIGNAL(readOnlyChanged(bool)), m_attachmentsModel, SLOT(setReadOnly(bool)));

    connect(m_ui->attachmentsView, &QAbstractItemView::doubleClicked, [this](const QModelIndex&) {
        m_readOnly ? previewSelectedAttachment() : editSelectedAttachment();
    });

    connect(m_ui->attachmentsView->itemDelegate(), &QAbstractItemDelegate::commitData, [this](QWidget* editor) {
        if (auto lineEdit = qobject_cast<QLineEdit*>(editor)) {
            auto index = m_attachmentsModel->rowByKey(lineEdit->text());
            m_ui->attachmentsView->setCurrentIndex(m_attachmentsModel->index(index, 0));
        }
    });

    connect(m_ui->saveAttachmentButton, SIGNAL(clicked()), SLOT(saveSelectedAttachments()));
    connect(m_ui->openAttachmentButton, SIGNAL(clicked()), SLOT(openSelectedAttachments()));
    connect(m_ui->addAttachmentButton, SIGNAL(clicked()), SLOT(insertAttachments()));
    connect(m_ui->editAttachmentButton, SIGNAL(clicked()), SLOT(editSelectedAttachment()));
    connect(m_ui->previewAttachmentButton, SIGNAL(clicked()), SLOT(previewSelectedAttachment()));
    connect(m_ui->removeAttachmentButton, SIGNAL(clicked()), SLOT(removeSelectedAttachments()));

    auto addButtonMenu = new QMenu(this);
    addButtonMenu->addAction(tr("New Text Document"), this, &EntryAttachmentsWidget::newAttachments);
    addButtonMenu->addAction(tr("Load from Disk…"), this, QOverload<>::of(&EntryAttachmentsWidget::insertAttachments));

    m_ui->addAttachmentButton->setMenu(addButtonMenu);

    updateButtonsVisible();
    updateButtonsEnabled();
}

EntryAttachmentsWidget::~EntryAttachmentsWidget() = default;

const EntryAttachments* EntryAttachmentsWidget::attachments() const
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

void EntryAttachmentsWidget::linkAttachments(EntryAttachments* attachments)
{
    unlinkAttachments();

    m_entryAttachments = attachments;
    m_attachmentsModel->setEntryAttachments(m_entryAttachments);

    if (m_entryAttachments) {
        connect(m_entryAttachments,
                SIGNAL(valueModifiedExternally(QString, QString)),
                this,
                SLOT(attachmentModifiedExternally(QString, QString)));
        connect(m_entryAttachments, SIGNAL(modified()), this, SIGNAL(widgetUpdated()));
    }
}

void EntryAttachmentsWidget::unlinkAttachments()
{
    if (m_entryAttachments) {
        m_entryAttachments->disconnect(this);
        m_entryAttachments = nullptr;
        m_attachmentsModel->setEntryAttachments(nullptr);
    }
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

void EntryAttachmentsWidget::insertAttachments()
{
    Q_ASSERT(m_entryAttachments);
    Q_ASSERT(!isReadOnly());
    if (isReadOnly()) {
        return;
    }

    QString defaultDirPath = FileDialog::getLastDir("attachments");
    const auto filenames = fileDialog()->getOpenFileNames(this, tr("Select files"), defaultDirPath);
    if (filenames.isEmpty()) {
        return;
    }
    const auto confirmedFileNames = confirmAttachmentSelection(filenames);
    if (confirmedFileNames.isEmpty()) {
        return;
    }
    // Save path to first filename
    FileDialog::saveLastDir("attachments", filenames[0]);
    QString errorMessage;
    if (!insertAttachments(confirmedFileNames, errorMessage)) {
        errorOccurred(errorMessage);
    }
    emit widgetUpdated();
}

void EntryAttachmentsWidget::newAttachments()
{
    Q_ASSERT(m_entryAttachments);
    Q_ASSERT(!isReadOnly());
    if (isReadOnly()) {
        return;
    }

    // Create a temporary file to allow the user to edit the attachment
    auto newFileName = generateUniqueName(tr("New Attachment"), m_entryAttachments->keys());
    m_entryAttachments->set(newFileName, QByteArray());

    auto currentIndex = m_attachmentsModel->index(m_attachmentsModel->rowByKey(newFileName), 0);
    m_ui->attachmentsView->setCurrentIndex(currentIndex);
    m_ui->attachmentsView->edit(currentIndex);
}

void EntryAttachmentsWidget::previewSelectedAttachment()
{
    Q_ASSERT(m_entryAttachments);

    const auto selectionModel = m_ui->attachmentsView->selectionModel();
    if (!selectionModel) {
        qWarning() << "Failed to preview an attachment: No selection model";
        return;
    }

    auto indexes = selectionModel->selectedIndexes();
    if (indexes.empty()) {
        qWarning() << "Failed to edit an attachment: No attachment selected";
        return;
    }

    const auto index = indexes.first();
    if (!index.isValid()) {
        qWarning() << "Failed to preview an attachment: Attachment not found";
        return;
    }

    // Set selection to the first
    m_ui->attachmentsView->setCurrentIndex(index);

    auto name = m_attachmentsModel->keyByIndex(index);
    auto data = m_entryAttachments->value(name);

    PreviewEntryAttachmentsDialog previewDialog(this);
    previewDialog.setAttachment({name, data});

    connect(&previewDialog, SIGNAL(openAttachment(QString)), SLOT(openSelectedAttachments()));
    connect(&previewDialog, SIGNAL(saveAttachment(QString)), SLOT(saveSelectedAttachments()));
    // Refresh the preview if the attachment changes
    connect(m_entryAttachments,
            &EntryAttachments::keyModified,
            &previewDialog,
            [&previewDialog, &name, this](const QString& key) {
                if (key == name) {
                    previewDialog.setAttachment({name, m_entryAttachments->value(name)});
                }
            });

    previewDialog.exec();

    // Set focus back to the widget to allow keyboard navigation
    setFocus();
}

void EntryAttachmentsWidget::editSelectedAttachment()
{
    Q_ASSERT(m_entryAttachments);

    const auto selectionModel = m_ui->attachmentsView->selectionModel();
    if (!selectionModel) {
        qWarning() << "Failed to edit an attachment: No selection model";
        return;
    }

    const auto selectedIndexes = selectionModel->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        qWarning() << "Failed to edit an attachment: No attachment selected";
        return;
    }

    const auto index = selectedIndexes.first();

    if (!index.isValid()) {
        qWarning() << "Failed to edit an attachment: Attachment not found";
        return;
    }

    // Set selection to the first
    m_ui->attachmentsView->setCurrentIndex(index);

    auto name = m_attachmentsModel->keyByIndex(index);
    auto data = m_entryAttachments->value(name);

    EditEntryAttachmentsDialog editDialog(this);
    editDialog.setAttachment({name, data});

    if (editDialog.exec() == QDialog::Accepted) {
        auto attachment = editDialog.getAttachment();

        // Edit dialog cannot change the name of the attachment
        if (attachment.name == name) {
            m_entryAttachments->set(attachment.name, attachment.data);
        }
    }

    // Set focus back to the widget to allow keyboard navigation
    setFocus();
}

void EntryAttachmentsWidget::removeSelectedAttachments()
{
    Q_ASSERT(m_entryAttachments);
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

void EntryAttachmentsWidget::saveSelectedAttachments()
{
    Q_ASSERT(m_entryAttachments);

    const QModelIndexList indexes = m_ui->attachmentsView->selectionModel()->selectedRows(0);
    if (indexes.isEmpty()) {
        return;
    }

    QString defaultDirPath = FileDialog::getLastDir("attachments");
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
    FileDialog::saveLastDir("attachments", saveDirPath);

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
        const bool saveOk = file.open(QIODevice::WriteOnly) && file.setPermissions(QFile::ReadUser | QFile::WriteUser)
                            && file.write(attachmentData) == attachmentData.size();
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
    if (!m_entryAttachments->openAttachment(m_attachmentsModel->keyByIndex(index), &errorMessage)) {
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
        if (!m_entryAttachments->openAttachment(m_attachmentsModel->keyByIndex(index), &errorMessage)) {
            const QString filename = m_attachmentsModel->keyByIndex(index);
            errors.append(QString("%1 - %2").arg(filename, errorMessage));
        }
    }

    if (!errors.isEmpty()) {
        errorOccurred(tr("Unable to open attachments:\n%1").arg(errors.join('\n')));
    }
}

void EntryAttachmentsWidget::updateButtonsEnabled()
{
    const auto selectionModel = m_ui->attachmentsView->selectionModel();
    const bool hasSelection = selectionModel && selectionModel->hasSelection();

    m_ui->addAttachmentButton->setEnabled(!m_readOnly);
    m_ui->removeAttachmentButton->setEnabled(hasSelection && !m_readOnly);

    m_ui->editAttachmentButton->setEnabled(hasSelection && !m_readOnly);
    if (const auto indexes = selectionModel ? selectionModel->selectedIndexes() : QModelIndexList{}; !indexes.empty()) {
        auto mimeType = Tools::getMimeType(m_entryAttachments->value(m_attachmentsModel->keyByIndex(indexes.first())));
        m_ui->editAttachmentButton->setEnabled(hasSelection && !m_readOnly && Tools::isTextMimeType(mimeType));
    }

    m_ui->saveAttachmentButton->setEnabled(hasSelection);
    m_ui->previewAttachmentButton->setEnabled(hasSelection);
    m_ui->openAttachmentButton->setEnabled(hasSelection);

    updateLinesVisibility();
}

void EntryAttachmentsWidget::updateLinesVisibility()
{
    m_ui->editPreviewLine->setVisible(m_buttonsVisible && !m_readOnly);
    m_ui->previewRemoveLine->setVisible(m_buttonsVisible && !m_readOnly);
}

void EntryAttachmentsWidget::updateButtonsVisible()
{
    m_ui->addAttachmentButton->setVisible(m_buttonsVisible && !m_readOnly);
    m_ui->editAttachmentButton->setVisible(m_buttonsVisible && !m_readOnly);
    m_ui->removeAttachmentButton->setVisible(m_buttonsVisible && !m_readOnly);

    updateLinesVisibility();
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

QStringList EntryAttachmentsWidget::confirmAttachmentSelection(const QStringList& filenames)
{
    QStringList confirmedFileNames;
    for (const auto& file : filenames) {
        const QFileInfo fileInfo(file);
        auto fileName = fileInfo.fileName();

        // Ask for confirmation if overwriting an existing attachment
        if (m_entryAttachments->hasKey(fileName)) {
            auto result = MessageBox::question(this,
                                               tr("Confirm Overwrite Attachment"),
                                               tr("Attachment \"%1\" already exists. \n"
                                                  "Would you like to overwrite the existing attachment?")
                                                   .arg(fileName),
                                               MessageBox::Overwrite | MessageBox::No,
                                               MessageBox::No);
            if (result == MessageBox::No) {
                continue;
            }
        }

        // Ask for confirmation before adding files over 5 MB in size
        double size = fileInfo.size() / (1024.0 * 1024.0);
        if (size > 5.0) {
            auto result =
                MessageBox::question(this,
                                     tr("Confirm Attachment"),
                                     tr("%1 is a big file (%2 MB).\nYour database may get very large and reduce "
                                        "performance.\n\nAre you sure to add this file?")
                                         .arg(fileName, QString::number(size, 'f', 1)),
                                     MessageBox::Yes | MessageBox::No,
                                     MessageBox::No);
            if (result == MessageBox::No) {
                continue;
            }
        }

        confirmedFileNames << file;
    }

    return confirmedFileNames;
}

bool EntryAttachmentsWidget::eventFilter(QObject* watched, QEvent* e)
{
    if (watched == m_ui->attachmentsView->viewport() && !isReadOnly()) {
        const QEvent::Type eventType = e->type();
        if (eventType == QEvent::DragEnter || eventType == QEvent::DragMove) {
            auto dropEv = static_cast<QDropEvent*>(e);
            const QMimeData* mimeData = dropEv->mimeData();
            if (mimeData->hasUrls()) {
                dropEv->acceptProposedAction();
                return true;
            }
        } else if (eventType == QEvent::Drop) {
            auto dropEv = static_cast<QDropEvent*>(e);
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

void EntryAttachmentsWidget::attachmentModifiedExternally(const QString& key, const QString& filePath)
{
    if (m_pendingChanges.contains(filePath)) {
        return;
    }

    m_pendingChanges << filePath;

    auto result = MessageBox::question(
        this,
        tr("Attachment modified"),
        tr("The attachment '%1' was modified.\nDo you want to save the changes to your database?").arg(key),
        MessageBox::Save | MessageBox::Discard,
        MessageBox::Save);

    if (result == MessageBox::Save) {
        QFile f(filePath);
        if (f.open(QFile::ReadOnly)) {
            m_entryAttachments->set(key, f.readAll());
            f.close();
            emit widgetUpdated();
        } else {
            MessageBox::critical(this,
                                 tr("Saving attachment failed"),
                                 tr("Saving updated attachment failed.\nError: %1").arg(f.errorString()));
        }
    }

    m_pendingChanges.removeAll(filePath);
}
