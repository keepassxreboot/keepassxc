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
#include <QDesktopServices>
#include <QDir>

#include "core/Config.h"
#include "core/FilePath.h"
#include "entry/EntryAttachmentsModel.h"
#include "gui/Clipboard.h"

namespace
{
    constexpr int GeneralTabIndex = 0;
}

DetailsWidget::DetailsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::DetailsWidget())
    , m_locked(false)
    , m_currentEntry(nullptr)
    , m_currentGroup(nullptr)
    , m_step(0)
    , m_totpTimer(nullptr)
    , m_selectedTabEntry(0)
    , m_selectedTabGroup(0)
{
    m_ui->setupUi(this);

    // Entry
    m_ui->entryTotpButton->setIcon(filePath()->icon("actions", "chronometer"));
    m_ui->entryCloseButton->setIcon(filePath()->icon("actions", "dialog-close"));

    m_ui->entryAttachmentsWidget->setReadOnly(true);
    m_ui->entryAttachmentsWidget->setButtonsVisible(false);

    connect(m_ui->entryTotpButton, SIGNAL(toggled(bool)), m_ui->entryTotpWidget, SLOT(setVisible(bool)));
    connect(m_ui->entryCloseButton, SIGNAL(toggled(bool)), SLOT(hide()));
    connect(m_ui->entryTabWidget, SIGNAL(tabBarClicked(int)), SLOT(updateTabIndexes()), Qt::QueuedConnection);

    // Group
    m_ui->groupCloseButton->setIcon(filePath()->icon("actions", "dialog-close"));
    connect(m_ui->groupCloseButton, SIGNAL(toggled(bool)), SLOT(hide()));
    connect(m_ui->groupTabWidget, SIGNAL(tabBarClicked(int)), SLOT(updateTabIndexes()), Qt::QueuedConnection);
}

DetailsWidget::~DetailsWidget()
{
    deleteTotpTimer();
}

void DetailsWidget::setEntry(Entry* selectedEntry)
{
    if (!selectedEntry) {
        hide();
        return;
    }

    m_currentEntry = selectedEntry;

    updateEntryHeaderLine();
    updateEntryTotp();
    updateEntryGeneralTab();
    updateEntryNotesTab();
    updateEntryAttributesTab();
    updateEntryAttachmentsTab();
    updateEntryAutotypeTab();

    setVisible(!config()->get("GUI/HideDetailsView").toBool());

    m_ui->stackedWidget->setCurrentWidget(m_ui->pageEntry);
    const int tabIndex = m_ui->entryTabWidget->isTabEnabled(m_selectedTabEntry) ? m_selectedTabEntry : GeneralTabIndex;
    Q_ASSERT(m_ui->entryTabWidget->isTabEnabled(GeneralTabIndex));
    m_ui->entryTabWidget->setCurrentIndex(tabIndex);
}

void DetailsWidget::setGroup(Group* selectedGroup)
{
    if (!selectedGroup) {
        hide();
        return;
    }

    m_currentGroup = selectedGroup;
    updateGroupHeaderLine();
    updateGroupGeneralTab();
    updateGroupNotesTab();

    setVisible(!config()->get("GUI/HideDetailsView").toBool());

    m_ui->stackedWidget->setCurrentWidget(m_ui->pageGroup);
    const int tabIndex = m_ui->groupTabWidget->isTabEnabled(m_selectedTabGroup) ? m_selectedTabGroup : GeneralTabIndex;
    Q_ASSERT(m_ui->groupTabWidget->isTabEnabled(GeneralTabIndex));
    m_ui->groupTabWidget->setCurrentIndex(tabIndex);
}

void DetailsWidget::setDatabaseMode(DatabaseWidget::Mode mode)
{
    m_locked = mode == DatabaseWidget::LockedMode;
    if (m_locked) {
        return;
    }

    if (mode == DatabaseWidget::ViewMode) {
        if (m_ui->stackedWidget->currentWidget() == m_ui->pageGroup) {
            setGroup(m_currentGroup);
        } else {
            setEntry(m_currentEntry);
        }
    }
}

void DetailsWidget::updateEntryHeaderLine()
{
    Q_ASSERT(m_currentEntry);
    const QString title = m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->title());
    m_ui->entryTitleLabel->setRawText(hierarchy(m_currentEntry->group(), title));
    m_ui->entryIcon->setPixmap(preparePixmap(m_currentEntry->iconPixmap(), 16));
}

void DetailsWidget::updateEntryTotp()
{
    Q_ASSERT(m_currentEntry);
    const bool hasTotp = m_currentEntry->hasTotp();
    m_ui->entryTotpButton->setVisible(hasTotp);
    m_ui->entryTotpWidget->hide();
    m_ui->entryTotpButton->setChecked(false);

    if (hasTotp) {
        deleteTotpTimer();
        m_totpTimer = new QTimer(m_currentEntry);
        connect(m_totpTimer, SIGNAL(timeout()), this, SLOT(updateTotpLabel()));
        m_totpTimer->start(1000);

        m_step = m_currentEntry->totpStep();
        updateTotpLabel();
    } else {
        m_ui->entryTotpLabel->clear();
        stopTotpTimer();
    }
}

void DetailsWidget::updateEntryGeneralTab()
{
    Q_ASSERT(m_currentEntry);
    m_ui->entryUsernameLabel->setText(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->username()));

    if (!config()->get("security/hidepassworddetails").toBool()) {
        const QString password = m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->password());
        m_ui->entryPasswordLabel->setRawText(password);
        m_ui->entryPasswordLabel->setToolTip(password);
    } else {
        m_ui->entryPasswordLabel->setRawText(QString("\u25cf").repeated(6));
        m_ui->entryPasswordLabel->setToolTip({});
    }

    const QString url = m_currentEntry->webUrl();
    if (!url.isEmpty()) {
        // URL is well formed and can be opened in a browser
        // create a new display url that masks password placeholders
        // the actual link will use the password
        m_ui->entryUrlLabel->setRawText(m_currentEntry->displayUrl());
        m_ui->entryUrlLabel->setUrl(url);
    } else {
        // Fallback to the raw url string
        m_ui->entryUrlLabel->setRawText(m_currentEntry->resolveMultiplePlaceholders(m_currentEntry->url()));
        m_ui->entryUrlLabel->setUrl({});
    }

    const TimeInfo entryTime = m_currentEntry->timeInfo();
    const QString expires =
        entryTime.expires() ? entryTime.expiryTime().toString(Qt::DefaultLocaleShortDate) : tr("Never");
    m_ui->entryExpirationLabel->setText(expires);
}

void DetailsWidget::updateEntryNotesTab()
{
    Q_ASSERT(m_currentEntry);
    const QString notes = m_currentEntry->notes();
    setTabEnabled(m_ui->entryTabWidget, m_ui->entryNotesTab, !notes.isEmpty());
    m_ui->entryNotesEdit->setText(notes);
}

void DetailsWidget::updateEntryAttributesTab()
{
    Q_ASSERT(m_currentEntry);
    m_ui->entryAttributesEdit->clear();
    const EntryAttributes* attributes = m_currentEntry->attributes();
    const QStringList customAttributes = attributes->customKeys();
    const bool haveAttributes = customAttributes.size() > 0;
    setTabEnabled(m_ui->entryTabWidget, m_ui->entryAttributesTab, haveAttributes);
    if (haveAttributes) {
        QString attributesText;
        for (const QString& key : customAttributes) {
            QString value = m_currentEntry->attributes()->value(key);
            if (m_currentEntry->attributes()->isProtected(key)) {
                value = "<i>" + tr("[PROTECTED]") + "</i>";
            }
            attributesText.append(tr("<b>%1</b>: %2", "attributes line").arg(key, value).append("<br/>"));
        }
        m_ui->entryAttributesEdit->setText(attributesText);
    }
}

void DetailsWidget::updateEntryAttachmentsTab()
{
    Q_ASSERT(m_currentEntry);
    const bool hasAttachments = !m_currentEntry->attachments()->isEmpty();
    setTabEnabled(m_ui->entryTabWidget, m_ui->entryAttachmentsTab, hasAttachments);
    m_ui->entryAttachmentsWidget->setEntryAttachments(m_currentEntry->attachments());
}

void DetailsWidget::updateEntryAutotypeTab()
{
    Q_ASSERT(m_currentEntry);
    m_ui->entryAutotypeTree->clear();
    QList<QTreeWidgetItem*> items;
    const AutoTypeAssociations* autotypeAssociations = m_currentEntry->autoTypeAssociations();
    const auto associations = autotypeAssociations->getAll();
    for (const auto& assoc : associations) {
        const QString sequence =
            assoc.sequence.isEmpty() ? m_currentEntry->effectiveAutoTypeSequence() : assoc.sequence;
        items.append(new QTreeWidgetItem(m_ui->entryAutotypeTree, {assoc.window, sequence}));
    }

    m_ui->entryAutotypeTree->addTopLevelItems(items);
    setTabEnabled(m_ui->entryTabWidget, m_ui->entryAutotypeTab, !items.isEmpty());
}

void DetailsWidget::updateGroupHeaderLine()
{
    Q_ASSERT(m_currentGroup);
    m_ui->groupTitleLabel->setRawText(hierarchy(m_currentGroup, {}));
    m_ui->groupIcon->setPixmap(preparePixmap(m_currentGroup->iconPixmap(), 32));
}

void DetailsWidget::updateGroupGeneralTab()
{
    Q_ASSERT(m_currentGroup);
    const QString searchingText = m_currentGroup->resolveSearchingEnabled() ? tr("Enabled") : tr("Disabled");
    m_ui->groupSearchingLabel->setText(searchingText);

    const QString autotypeText = m_currentGroup->resolveAutoTypeEnabled() ? tr("Enabled") : tr("Disabled");
    m_ui->groupAutotypeLabel->setText(autotypeText);

    const TimeInfo groupTime = m_currentGroup->timeInfo();
    const QString expiresText =
        groupTime.expires() ? groupTime.expiryTime().toString(Qt::DefaultLocaleShortDate) : tr("Never");
    m_ui->groupExpirationLabel->setText(expiresText);
}

void DetailsWidget::updateGroupNotesTab()
{
    Q_ASSERT(m_currentGroup);
    const QString notes = m_currentGroup->notes();
    setTabEnabled(m_ui->groupTabWidget, m_ui->groupNotesTab, !notes.isEmpty());
    m_ui->groupNotesEdit->setText(notes);
}

void DetailsWidget::stopTotpTimer()
{
    if (m_totpTimer) {
        m_totpTimer->stop();
    }
}

void DetailsWidget::deleteTotpTimer()
{
    if (m_totpTimer) {
        delete m_totpTimer;
    }
}

void DetailsWidget::updateTotpLabel()
{
    if (!m_locked && m_currentEntry) {
        const QString totpCode = m_currentEntry->totp();
        const QString firstHalf = totpCode.left(totpCode.size() / 2);
        const QString secondHalf = totpCode.mid(totpCode.size() / 2);
        m_ui->entryTotpLabel->setText(firstHalf + " " + secondHalf);
    } else {
        m_ui->entryTotpLabel->clear();
        stopTotpTimer();
    }
}

void DetailsWidget::updateTabIndexes()
{
    m_selectedTabEntry = m_ui->entryTabWidget->currentIndex();
    m_selectedTabGroup = m_ui->groupTabWidget->currentIndex();
}

void DetailsWidget::setTabEnabled(QTabWidget* tabWidget, QWidget* widget, bool enabled)
{
    const int tabIndex = tabWidget->indexOf(widget);
    Q_ASSERT(tabIndex != -1);
    tabWidget->setTabEnabled(tabIndex, enabled);
}

QPixmap DetailsWidget::preparePixmap(const QPixmap& pixmap, int size)
{
    if (pixmap.width() > size || pixmap.height() > size) {
        return pixmap.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return pixmap;
}

QString DetailsWidget::hierarchy(const Group* group, const QString& title)
{
    const QString separator(" / ");
    QStringList hierarchy = group->hierarchy();
    hierarchy.removeFirst();
    hierarchy.append(title);
    return QString("%1%2").arg(separator, hierarchy.join(separator));
}
