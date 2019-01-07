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
    m_ui->messageWidget->setCloseButtonVisible(false);
    m_ui->messageWidget->setAutoHideTimeout(-1);

    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), m_ui->passwordEdit, SLOT(setShowPassword(bool)));
    connect(m_ui->togglePasswordGeneratorButton, SIGNAL(toggled(bool)), SLOT(togglePasswordGeneratorButton(bool)));
    connect(m_ui->passwordEdit, SIGNAL(textChanged(QString)), SLOT(selectPassword()));
    connect(m_ui->passwordGenerator, SIGNAL(appliedPassword(QString)), SLOT(setGeneratedPassword(QString)));
    connect(m_ui->pathEdit, SIGNAL(editingFinished()), SLOT(selectPath()));
    connect(m_ui->pathSelectionButton, SIGNAL(pressed()), SLOT(launchPathSelectionDialog()));
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
        connect(m_temporaryGroup, SIGNAL(groupModified()), SLOT(update()));
    }

    update();
}

void EditGroupWidgetKeeShare::showSharingState()
{
    if (!m_temporaryGroup) {
        return;
    }

    auto supportedExtensions = QStringList();
#if defined(WITH_XC_KEESHARE_INSECURE)
    supportedExtensions << KeeShare::insecureContainerFileType();
#endif
#if defined(WITH_XC_KEESHARE_SECURE)
    supportedExtensions << KeeShare::secureContainerFileType();
#endif
    const auto reference = KeeShare::referenceOf(m_temporaryGroup);
    if (!reference.path.isEmpty()) {
        bool supported = false;
        for(const auto &extension : supportedExtensions){
            if (reference.path.endsWith(extension, Qt::CaseInsensitive)){
                supported = true;
                break;
            }
        }
        if (!supported) {
            m_ui->messageWidget->showMessage(
                    tr("Your KeePassXC version does not support sharing your container type. Please use %1.")
                        .arg(supportedExtensions.join(", ")),
                    MessageWidget::Warning);
            return;
        }
    }
    const auto active = KeeShare::active();
    if (!active.in && !active.out) {
        m_ui->messageWidget->showMessage(tr("Database sharing is disabled"), MessageWidget::Information);
        return;
    }
    if (active.in && !active.out) {
        m_ui->messageWidget->showMessage(tr("Database export is disabled"), MessageWidget::Information);
        return;
    }
    if (!active.in && active.out) {
        m_ui->messageWidget->showMessage(tr("Database import is disabled"), MessageWidget::Information);
        return;
    }
}

void EditGroupWidgetKeeShare::update()
{
    if (!m_temporaryGroup) {
        m_ui->passwordEdit->clear();
        m_ui->pathEdit->clear();
    } else {
        const auto reference = KeeShare::referenceOf(m_temporaryGroup);

        m_ui->typeComboBox->setCurrentIndex(reference.type);
        m_ui->passwordEdit->setText(reference.password);
        m_ui->pathEdit->setText(reference.path);

        showSharingState();
    }

    m_ui->passwordGenerator->hide();
    m_ui->togglePasswordGeneratorButton->setChecked(false);
    m_ui->togglePasswordButton->setChecked(false);
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

void EditGroupWidgetKeeShare::selectPath()
{
    if (!m_temporaryGroup) {
        return;
    }
    auto reference = KeeShare::referenceOf(m_temporaryGroup);
    reference.path = m_ui->pathEdit->text();
    KeeShare::setReferenceTo(m_temporaryGroup, reference);
}

void EditGroupWidgetKeeShare::launchPathSelectionDialog()
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
    QString defaultFiletype = "";
    auto supportedExtensions = QStringList();
    auto unsupportedExtensions = QStringList();
    auto knownFilters = QStringList() << QString("%1 (*)").arg("All files");
#if defined(WITH_XC_KEESHARE_INSECURE)
    defaultFiletype = KeeShare::insecureContainerFileType();
    supportedExtensions << KeeShare::insecureContainerFileType();
    knownFilters.prepend(QString("%1 (*.%2)").arg(tr("KeeShare insecure container"), KeeShare::insecureContainerFileType()));
#else
    unsupportedExtensions << KeeShare::insecureContainerFileType();
#endif
#if defined(WITH_XC_KEESHARE_SECURE)
    defaultFiletype = KeeShare::secureContainerFileType();
    supportedExtensions << KeeShare::secureContainerFileType();
    knownFilters.prepend(QString("%1 (*.%2)").arg(tr("KeeShare secure container"), KeeShare::secureContainerFileType()));
#else
    unsupportedExtensions << KeeShare::secureContainerFileType();
#endif

    const auto filters = knownFilters.join(";;");
    auto filename = reference.path;
    if (filename.isEmpty()) {
        filename = m_temporaryGroup->name();
    }
    switch (reference.type) {
    case KeeShareSettings::ImportFrom:
        filename = fileDialog()->getFileName(
            this, tr("Select import source"), defaultDirPath, filters, nullptr, QFileDialog::DontConfirmOverwrite,
            defaultFiletype, filename);
        break;
    case KeeShareSettings::ExportTo:
        filename = fileDialog()->getFileName(
            this, tr("Select export target"), defaultDirPath, filters, nullptr, QFileDialog::Option(0), defaultFiletype, filename);
        break;
    case KeeShareSettings::SynchronizeWith:
    case KeeShareSettings::Inactive:
        filename = fileDialog()->getFileName(
            this, tr("Select import/export file"), defaultDirPath, filters, nullptr, QFileDialog::Option(0), defaultFiletype, filename);
        break;
    }

    if (filename.isEmpty()) {
        return;
    }
    bool validFilename = false;
    for(const auto& extension : supportedExtensions + unsupportedExtensions){
        if (filename.endsWith(extension, Qt::CaseInsensitive)) {
            validFilename = true;
            break;
        }
    }
    if (!validFilename){
        filename += (!filename.endsWith(".") ? "." : "") + defaultFiletype;
    }

    m_ui->pathEdit->setText(filename);
    selectPath();
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
