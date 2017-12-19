/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "DetailsWidget.h"
#include "ui_DetailsWidget.h"

#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QDesktopServices>
#include <QTemporaryFile>

#include "core/Config.h"
#include "core/FilePath.h"
#include "core/TimeInfo.h"
#include "gui/Clipboard.h"
#include "gui/DatabaseWidget.h"
#include "entry/EntryAttachmentsModel.h"

DetailsWidget::DetailsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::DetailsWidget())
    , m_locked(false)
    , m_currentEntry(nullptr)
    , m_currentGroup(nullptr)
    , m_timer(nullptr)
    , m_attributesWidget(nullptr)
    , m_attachmentsWidget(nullptr)
    , m_autotypeWidget(nullptr)
    , m_selectedTabEntry(0)
    , m_selectedTabGroup(0)
    , m_attachmentsModel(new EntryAttachmentsModel(this))
{
    m_ui->setupUi(this);

    connect(parent, SIGNAL(pressedEntry(Entry*)), SLOT(getSelectedEntry(Entry*)));
    connect(parent, SIGNAL(pressedGroup(Group*)), SLOT(getSelectedGroup(Group*)));
    connect(parent, SIGNAL(currentModeChanged(DatabaseWidget::Mode)), SLOT(setDatabaseMode(DatabaseWidget::Mode)));

    m_ui->totpButton->setIcon(filePath()->icon("actions", "chronometer"));
    m_ui->closeButton->setIcon(filePath()->icon("actions", "dialog-close"));

    connect(m_ui->totpButton, SIGNAL(toggled(bool)), SLOT(showTotp(bool)));
    connect(m_ui->closeButton, SIGNAL(toggled(bool)), SLOT(hideDetails()));
    connect(m_ui->tabWidget, SIGNAL(tabBarClicked(int)), SLOT(updateTabIndex(int)));

    m_ui->attachmentsTableView->setModel(m_attachmentsModel);
    m_ui->attachmentsTableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->attachmentsTableView->horizontalHeader()->resizeSection(0, 600);
    m_ui->attachmentsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->attachmentsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_ui->attachmentsTableView, SIGNAL(activated(QModelIndex)), SLOT(openAttachment(QModelIndex)));

    m_attributesWidget = m_ui->tabWidget->widget(AttributesTab);
    m_attachmentsWidget = m_ui->tabWidget->widget(AttachmentsTab);
    m_autotypeWidget = m_ui->tabWidget->widget(AutotypeTab);

    this->hide();
}

DetailsWidget::~DetailsWidget()
{
}

void DetailsWidget::getSelectedEntry(Entry* selectedEntry)
{
    if (!selectedEntry) {
        hideDetails();
        return;
    }

    m_currentEntry = selectedEntry;

    if (!config()->get("GUI/HideDetailsView").toBool()) {
        this->show();
    }

    m_ui->stackedWidget->setCurrentIndex(EntryPreview);

    if (m_ui->tabWidget->count() < 5) {
        m_ui->tabWidget->insertTab(static_cast<int>(AttributesTab), m_attributesWidget, "Attributes");
        m_ui->tabWidget->insertTab(static_cast<int>(AttachmentsTab), m_attachmentsWidget, "Attachments");
        m_ui->tabWidget->insertTab(static_cast<int>(AutotypeTab), m_autotypeWidget, "Autotype");
    }

    m_ui->tabWidget->setTabEnabled(AttributesTab, false);
    m_ui->tabWidget->setTabEnabled(NotesTab, false);
    m_ui->tabWidget->setTabEnabled(AutotypeTab, false);

    m_ui->totpButton->hide();
    m_ui->totpWidget->hide();
    m_ui->totpButton->setChecked(false);

    auto icon = m_currentEntry->iconPixmap();
    if (icon.width() > 16 || icon.height() > 16) {
        icon = icon.scaled(16, 16);
    }
    m_ui->entryIcon->setPixmap(icon);

    QString title = QString(" / ");
    Group* entry_group = m_currentEntry->group();
    if (entry_group) {
        QStringList hierarchy = entry_group->hierarchy();
        hierarchy.removeFirst();
        title += hierarchy.join(" / ");
        if (hierarchy.size() > 0) {
            title += " / ";
        }
    }
    title.append(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->title()));
    m_ui->titleLabel->setText(title);

    m_ui->usernameLabel->setText(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->username()));

    if (!config()->get("security/hidepassworddetails").toBool()) {
        m_ui->passwordLabel->setText(
            shortPassword(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->password())));
        m_ui->passwordLabel->setToolTip(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->password()));
    } else {
        m_ui->passwordLabel->setText("****");
    }

    QString url = m_currentEntry->webUrl();
    if (!url.isEmpty()) {
        // URL is well formed and can be opened in a browser
        // create a new display url that masks password placeholders
        // the actual link will use the password
        url = QString("<a href=\"%1\">%2</a>").arg(url).arg(shortUrl(m_currentEntry->displayUrl()));
        m_ui->urlLabel->setOpenExternalLinks(true);
    } else {
        // Fallback to the raw url string
        url = shortUrl(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->url()));
        m_ui->urlLabel->setOpenExternalLinks(false);
    }
    m_ui->urlLabel->setText(url);

    TimeInfo entryTime = m_currentEntry->timeInfo();
    if (entryTime.expires()) {
        m_ui->expirationLabel->setText(entryTime.expiryTime().toString(Qt::DefaultLocaleShortDate));
    } else {
        m_ui->expirationLabel->setText(tr("Never"));
    }

    if (m_currentEntry->hasTotp()) {
        m_step = m_currentEntry->totpStep();

        if (nullptr != m_timer) {
            m_timer->stop();
        }
        m_timer = new QTimer(this);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(updateTotp()));
        updateTotp();
        m_timer->start(m_step * 10);
        m_ui->totpButton->show();
    }

    QString notes = m_currentEntry->notes();
    if (!notes.isEmpty()) {
        m_ui->tabWidget->setTabEnabled(NotesTab, true);
        m_ui->notesEdit->setText(m_currentEntry->resolveMultiplePlaceholders(notes));
    }

    QStringList customAttributes = m_currentEntry->attributes()->customKeys();
    if (customAttributes.size() > 0) {
        m_ui->tabWidget->setTabEnabled(AttributesTab, true);
        m_ui->attributesEdit->clear();

        QString attributesText = QString();
        for (const QString& key : customAttributes) {
            QString value = m_currentEntry->attributes()->value(key);
            if (m_currentEntry->attributes()->isProtected(key)) {
                value = "<i>" + tr("[PROTECTED]") + "</i>";
            }
            attributesText.append(QString("<b>%1</b>: %2<br/>").arg(key, value));
        }
        m_ui->attributesEdit->setText(attributesText);
    }

    const bool hasAttachments = !m_currentEntry->attachments()->isEmpty();
    m_ui->tabWidget->setTabEnabled(AttachmentsTab, hasAttachments);
    m_attachmentsModel->setEntryAttachments(hasAttachments ? m_currentEntry->attachments() : nullptr);

    m_ui->autotypeTree->clear();
    AutoTypeAssociations* autotypeAssociations = m_currentEntry->autoTypeAssociations();
    QList<QTreeWidgetItem*> items;
    for (auto assoc : autotypeAssociations->getAll()) {
        QStringList association = QStringList() << assoc.window << assoc.sequence;
        if (association.at(1).isEmpty()) {
            association.replace(1, m_currentEntry->effectiveAutoTypeSequence());
        }
        items.append(new QTreeWidgetItem(m_ui->autotypeTree, association));
    }
    if (items.count() > 0) {
        m_ui->autotypeTree->addTopLevelItems(items);
        m_ui->tabWidget->setTabEnabled(AutotypeTab, true);
    }

    if (m_ui->tabWidget->isTabEnabled(m_selectedTabEntry)) {
        m_ui->tabWidget->setCurrentIndex(m_selectedTabEntry);
    }
}

void DetailsWidget::getSelectedGroup(Group* selectedGroup)
{
    if (!selectedGroup) {
        hideDetails();
        return;
    }

    m_currentGroup = selectedGroup;

    if (!config()->get("GUI/HideDetailsView").toBool()) {
        this->show();
    }

    m_ui->stackedWidget->setCurrentIndex(GroupPreview);

    if (m_ui->tabWidget->count() > 2) {
        m_ui->tabWidget->removeTab(AutotypeTab);
        m_ui->tabWidget->removeTab(AttachmentsTab);
        m_ui->tabWidget->removeTab(AttributesTab);
    }

    m_ui->tabWidget->setTabEnabled(GroupNotesTab, false);

    m_ui->totpButton->hide();
    m_ui->totpWidget->hide();

    auto icon = m_currentGroup->iconPixmap();
    if (icon.width() > 32 || icon.height() > 32) {
        icon = icon.scaled(32, 32);
    }
    m_ui->entryIcon->setPixmap(icon);

    QString title = " / ";
    QStringList hierarchy = m_currentGroup->hierarchy();
    hierarchy.removeFirst();
    title += hierarchy.join(" / ");
    if (hierarchy.size() > 0) {
        title += " / ";
    }
    m_ui->titleLabel->setText(title);

    QString notes = m_currentGroup->notes();
    if (!notes.isEmpty()) {
        m_ui->tabWidget->setTabEnabled(GroupNotesTab, true);
        m_ui->notesEdit->setText(notes);
    }

    QString searching = tr("Disabled");
    if (m_currentGroup->resolveSearchingEnabled()) {
        searching = tr("Enabled");
    }
    m_ui->searchingLabel->setText(searching);

    QString autotype = tr("Disabled");
    if (m_currentGroup->resolveAutoTypeEnabled()) {
        autotype = tr("Enabled");
    }
    m_ui->autotypeLabel->setText(autotype);

    TimeInfo groupTime = m_currentGroup->timeInfo();
    if (groupTime.expires()) {
        m_ui->groupExpirationLabel->setText(groupTime.expiryTime().toString(Qt::DefaultLocaleShortDate));
    } else {
        m_ui->groupExpirationLabel->setText(tr("Never"));
    }

    if (m_ui->tabWidget->isTabEnabled(m_selectedTabGroup)) {
        m_ui->tabWidget->setCurrentIndex(m_selectedTabGroup);
    }
}

void DetailsWidget::updateTotp()
{
    if (!m_locked) {
        QString totpCode = m_currentEntry->totp();
        QString firstHalf = totpCode.left(totpCode.size() / 2);
        QString secondHalf = totpCode.mid(totpCode.size() / 2);
        m_ui->totpLabel->setText(firstHalf + " " + secondHalf);
    } else if (nullptr != m_timer) {
        m_timer->stop();
    }
}

void DetailsWidget::showTotp(bool visible)
{
    if (visible) {
        m_ui->totpWidget->show();
    } else {
        m_ui->totpWidget->hide();
    }
}

QString DetailsWidget::shortUrl(QString url)
{
    QString newurl = "";
    if (url.length() > 60) {
        newurl.append(url.left(20));
        newurl.append("…");
        newurl.append(url.right(20));
        return newurl;
    }
    return url;
}

QString DetailsWidget::shortPassword(QString password)
{
    QString newpassword = "";
    if (password.length() > 60) {
        newpassword.append(password.left(50));
        newpassword.append("…");
        return newpassword;
    }
    return password;
}

void DetailsWidget::hideDetails()
{
    this->hide();
}

void DetailsWidget::setDatabaseMode(DatabaseWidget::Mode mode)
{
    m_locked = false;
    if (mode == DatabaseWidget::LockedMode) {
        m_locked = true;
        return;
    }
    if (mode == DatabaseWidget::ViewMode) {
        if (m_ui->stackedWidget->currentIndex() == GroupPreview) {
            getSelectedGroup(m_currentGroup);
        } else {
            getSelectedEntry(m_currentEntry);
        }
    }
}

void DetailsWidget::updateTabIndex(int index)
{
    if (m_ui->stackedWidget->currentIndex() == GroupPreview) {
        m_selectedTabGroup = index;
    } else {
        m_selectedTabEntry = index;
    }
}

void DetailsWidget::openAttachment(const QModelIndex& index)
{
    Q_ASSERT(m_currentEntry != nullptr);
    const QString filename = m_attachmentsModel->keyByIndex(index);
    const QByteArray attachmentData = m_currentEntry->attachments()->value(filename);

    // tmp file will be removed once the database (or the application) has been closed
    const QString tmpFileTemplate = QDir::temp().absoluteFilePath(QString("XXXXXX.").append(filename));
    QTemporaryFile* tmpFile = new QTemporaryFile(tmpFileTemplate, this);

    const bool saveOk = tmpFile->open()
                        && tmpFile->write(attachmentData) == attachmentData.size()
                        && tmpFile->flush();

    if (!saveOk) {
        delete tmpFile;
        emit errorOccurred(tr("Unable to open the attachment:\n").append(tmpFile->errorString()));
        return;
    }

    tmpFile->close();
    QDesktopServices::openUrl(QUrl::fromLocalFile(tmpFile->fileName()));
}

