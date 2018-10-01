/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "EditGroupWidgetKeeShare.h"
#include "ui_EditGroupWidgetKeeShare.h"

#include "core/Config.h"
#include "core/CustomData.h"
#include "core/FilePath.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/ssh/OpenSSHKey.h"
#include "gui/FileDialog.h"
#include "keeshare/KeeShare.h"

#include <QDir>
#include <QStandardPaths>

EditGroupWidgetKeeShare::EditGroupWidgetKeeShare(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EditGroupWidgetKeeShare())
{
    m_ui->setupUi(this);

    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    m_ui->togglePasswordGeneratorButton->setIcon(filePath()->icon("actions", "password-generator", false));

    m_ui->passwordGenerator->layout()->setContentsMargins(0, 0, 0, 0);
    m_ui->passwordGenerator->hide();
    m_ui->passwordGenerator->reset();

    m_ui->messageWidget->hide();

    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), m_ui->passwordEdit, SLOT(setShowPassword(bool)));
    connect(m_ui->togglePasswordGeneratorButton, SIGNAL(toggled(bool)), SLOT(togglePasswordGeneratorButton(bool)));
    connect(m_ui->passwordEdit, SIGNAL(textChanged(QString)), SLOT(selectPassword()));
    connect(m_ui->passwordGenerator, SIGNAL(appliedPassword(QString)), SLOT(setGeneratedPassword(QString)));
    connect(m_ui->pathEdit, SIGNAL(textChanged(QString)), SLOT(setPath(QString)));
    connect(m_ui->pathSelectionButton, SIGNAL(pressed()), SLOT(selectPath()));
    connect(m_ui->typeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(selectType()));

    connect(KeeShare::instance(), SIGNAL(activeChanged()), SLOT(showSharingState()));

    const auto types = QList<KeeShareSettings::Type>() << KeeShareSettings::Inactive 
                                                        << KeeShareSettings::ImportFrom
                                                        << KeeShareSettings::ExportTo
                                                        << KeeShareSettings::SynchronizeWith;
    for (const auto& type : types) {
        QString name;
        switch (type) {
        case KeeShareSettings::Inactive:
            name = tr("Inactive");
            break;
        case KeeShareSettings::ImportFrom:
            name = tr("Import from path");
            break;
        case KeeShareSettings::ExportTo:
            name = tr("Export to path");
            break;
        case KeeShareSettings::SynchronizeWith:
            name = tr("Synchronize with path");
            break;
        }
        m_ui->typeComboBox->insertItem(type, name, static_cast<int>(type));
    }
}

EditGroupWidgetKeeShare::~EditGroupWidgetKeeShare()
{
}

void EditGroupWidgetKeeShare::setGroup(Group* temporaryGroup)
{
    if (m_temporaryGroup) {
        m_temporaryGroup->disconnect(this);
    }

    m_temporaryGroup = temporaryGroup;

    if (m_temporaryGroup) {
        connect(m_temporaryGroup, SIGNAL(modified()), SLOT(update()));
    }
    update();
}

void EditGroupWidgetKeeShare::showSharingState()
{
    if (!m_temporaryGroup) {
        return;
    }
    const auto active = KeeShare::active();
    if (!active.in && !active.out) {
        m_ui->messageWidget->showMessage(tr("Database sharing is disabled"), MessageWidget::Information);
    }
    if (active.in && !active.out) {
        m_ui->messageWidget->showMessage(tr("Database export is disabled"), MessageWidget::Information);
    }
    if (!active.in && active.out) {
        m_ui->messageWidget->showMessage(tr("Database import is disabled"), MessageWidget::Information);
    }
}

void EditGroupWidgetKeeShare::update()
{
    if (!m_temporaryGroup) {
        m_ui->passwordEdit->clear();
        m_ui->pathEdit->clear();
        m_ui->passwordGenerator->hide();
        m_ui->togglePasswordGeneratorButton->setChecked(false);

    } else {
        const auto reference = KeeShare::referenceOf(m_temporaryGroup);

        m_ui->typeComboBox->setCurrentIndex(reference.type);
        m_ui->passwordEdit->setText(reference.password);
        m_ui->pathEdit->setText(reference.path);

        showSharingState();
    }
}

void EditGroupWidgetKeeShare::togglePasswordGeneratorButton(bool checked)
{
    m_ui->passwordGenerator->regeneratePassword();
    m_ui->passwordGenerator->setVisible(checked);
}

void EditGroupWidgetKeeShare::setGeneratedPassword(const QString& password)
{
    if (!m_temporaryGroup) {
        return;
    }
    auto reference = KeeShare::referenceOf(m_temporaryGroup);
    reference.password = password;
    KeeShare::setReferenceTo(m_temporaryGroup, reference);
    m_ui->togglePasswordGeneratorButton->setChecked(false);
}

void EditGroupWidgetKeeShare::setPath(const QString& path)
{
    if (!m_temporaryGroup) {
        return;
    }
    auto reference = KeeShare::referenceOf(m_temporaryGroup);
    reference.path = path;
    KeeShare::setReferenceTo(m_temporaryGroup, reference);
}

void EditGroupWidgetKeeShare::selectPath()
{
    if (!m_temporaryGroup) {
        return;
    }
    QString defaultDirPath = config()->get("KeeShare/LastShareDir").toString();
    const bool dirExists = !defaultDirPath.isEmpty() && QDir(defaultDirPath).exists();
    if (!dirExists) {
        defaultDirPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
    }
    auto reference = KeeShare::referenceOf(m_temporaryGroup);
    const auto filetype = tr("kdbx.share", "Filetype for KeeShare container");
    const auto filters = QString("%1 (*." + filetype + ");;%2 (*)").arg(tr("KeeShare Container"), tr("All files"));
    auto filename = reference.path;
    if (filename.isEmpty()) {
        filename = tr("%1.%2", "Template for KeeShare container").arg(m_temporaryGroup->name()).arg(filetype);
    }
    switch (reference.type) {
    case KeeShareSettings::ImportFrom:
        filename = fileDialog()->getFileName(this,
                                             tr("Select import source"),
                                             defaultDirPath,
                                             filters,
                                             nullptr,
                                             QFileDialog::DontConfirmOverwrite,
                                             filetype,
                                             filename);
        break;
    case KeeShareSettings::ExportTo:
        filename = fileDialog()->getFileName(
            this, tr("Select export target"), defaultDirPath, filters, nullptr, 0, filetype, filename);
        break;
    case KeeShareSettings::SynchronizeWith:
    case KeeShareSettings::Inactive:
        filename = fileDialog()->getFileName(
            this, tr("Select import/export file"), defaultDirPath, filters, nullptr, 0, filetype, filename);
        break;
    }

    if (filename.isEmpty()) {
        return;
    }

    setPath(filename);
    config()->set("KeeShare/LastShareDir", QFileInfo(filename).absolutePath());
}

void EditGroupWidgetKeeShare::selectPassword()
{
    if (!m_temporaryGroup) {
        return;
    }
    auto reference = KeeShare::referenceOf(m_temporaryGroup);
    reference.password = m_ui->passwordEdit->text();
    KeeShare::setReferenceTo(m_temporaryGroup, reference);
}

void EditGroupWidgetKeeShare::selectType()
{
    if (!m_temporaryGroup) {
        return;
    }
    auto reference = KeeShare::referenceOf(m_temporaryGroup);
    reference.type = static_cast<KeeShareSettings::Type>(m_ui->typeComboBox->currentData().toInt());
    KeeShare::setReferenceTo(m_temporaryGroup, reference);
}
