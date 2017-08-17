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

#include "core/Config.h"
#include "core/FilePath.h"
#include "core/TimeInfo.h"

DetailsWidget::DetailsWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::DetailsWidget())
    , m_currentEntry(nullptr)
    , m_currentGroup(nullptr)
{
    m_ui->setupUi(this);

    connect(parent, SIGNAL(pressedEntry(Entry*)), SLOT(getSelectedEntry(Entry*)));
    connect(parent, SIGNAL(pressedGroup(Group*)), SLOT(getSelectedGroup(Group*)));

    m_ui->totpButton->setIcon(filePath()->icon("actions", "chronometer"));
    m_ui->closeButton->setIcon(filePath()->icon("actions", "dialog-close"));
    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));

    connect(m_ui->totpButton, SIGNAL(toggled(bool)), SLOT(showTotp(bool)));
    connect(m_ui->closeButton, SIGNAL(toggled(bool)), SLOT(hideDetails()));
    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), SLOT(togglePasswordShown(bool)));

    this->hide();
}

DetailsWidget::~DetailsWidget()
{
}

void DetailsWidget::getSelectedEntry(Entry* selectedEntry) 
{
    m_currentEntry = selectedEntry;

    if (!config()->get("GUI/HideDetailsView").toBool()) {
        this->show();
    }

    m_ui->stackedWidget->setCurrentIndex(EntryPreview);

    m_ui->totpButton->hide();
    m_ui->totpWidget->hide();
    m_ui->totpButton->setChecked(false);

    m_ui->entryIcon->setPixmap(m_currentEntry->iconPixmap());

    QStringList hierarchy = m_currentEntry->group()->hierarchy();
    QString title = " / ";
    for (QString parent : hierarchy) {
        title.append(parent);
        title.append(" / ");
    }
    title.append(m_currentEntry->title());
    m_ui->titleLabel->setText(title);

    m_ui->usernameLabel->setText(m_currentEntry->username());
    m_ui->groupLabel->setText(m_currentEntry->group()->name());
    m_ui->notesEdit->setText(m_currentEntry->notes());

    if (!config()->get("security/hidepassworddetails").toBool()) {
        m_ui->passwordEdit->setText(m_currentEntry->password());
        m_ui->togglePasswordButton->show();
    } else {
        m_ui->passwordEdit->setText("****");
        m_ui->togglePasswordButton->hide();
    }

    QString url = m_currentEntry->webUrl();
    if (url != "") {
        url = QString("<a href=\"%1\">%2</a>").arg(url).arg(shortUrl(url));
        m_ui->urlLabel->setOpenExternalLinks(true);
    } else {
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

    m_ui->creationLabel->setText(entryTime.creationTime().toString(Qt::DefaultLocaleShortDate));
    m_ui->modifyLabel->setText(entryTime.lastModificationTime().toString(Qt::DefaultLocaleShortDate));
    m_ui->accessLabel->setText(entryTime.lastAccessTime().toString(Qt::DefaultLocaleShortDate));

    if (m_currentEntry->hasTotp()) {
        m_ui->totpButton->show();
        updateTotp();

        m_step = m_currentEntry->totpStep();

        m_timer = new QTimer(this);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(updateTotp()));
        m_timer->start(m_step * 10);
    }
}

void DetailsWidget::getSelectedGroup(Group* selectedGroup) 
{
    m_currentGroup = selectedGroup;

    if (!config()->get("GUI/HideDetailsView").toBool()) {
        this->show();
    }

    m_ui->stackedWidget->setCurrentIndex(GroupPreview);

    m_ui->totpButton->hide();
    m_ui->totpWidget->hide();

    m_ui->entryIcon->setPixmap(m_currentGroup->iconPixmap());

    QStringList hierarchy = m_currentGroup->hierarchy();
    QString title = " / ";
    for (QString parent : hierarchy) {
        title.append(parent);
        title.append(" / ");
    }
    m_ui->titleLabel->setText(title);

    m_ui->notesEdit->setText(m_currentGroup->notes());

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

    m_ui->groupCreationLabel->setText(groupTime.creationTime().toString(Qt::DefaultLocaleShortDate));
    m_ui->groupModifyLabel->setText(groupTime.lastModificationTime().toString(Qt::DefaultLocaleShortDate));
    m_ui->groupAccessLabel->setText(groupTime.lastAccessTime().toString(Qt::DefaultLocaleShortDate));
}

void DetailsWidget::updateTotp()
{
    QString totpCode = m_currentEntry->totp();
    QString firstHalf = totpCode.left(totpCode.size()/2);
    QString secondHalf = totpCode.right(totpCode.size()/2);
    m_ui->totpLabel->setText(firstHalf + " " + secondHalf);
}

void DetailsWidget::showTotp(bool visible)
{   
    if (visible){
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
        newurl.append("...");
        newurl.append(url.right(20));
        return newurl;
    }
    return url;
}

void DetailsWidget::hideDetails()
{
    this->hide();
}

void DetailsWidget::togglePasswordShown(bool showing)
{
    m_ui->passwordEdit->setShowPassword(showing);
    bool blockSignals = m_ui->togglePasswordButton->blockSignals(true);
    m_ui->togglePasswordButton->setChecked(showing);
    m_ui->togglePasswordButton->blockSignals(blockSignals);
}