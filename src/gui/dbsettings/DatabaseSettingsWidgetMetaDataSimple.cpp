/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseSettingsWidgetMetaDataSimple.h"
#include "ui_DatabaseSettingsWidgetMetaDataSimple.h"

#include "core/Database.h"
#include "core/Metadata.h"

#include <QButtonGroup>
#include <QStyle>

DatabaseSettingWidgetMetaData::DatabaseSettingWidgetMetaData(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidgetMetaDataSimple())
    , m_storageGroup(new QButtonGroup(this))
{
    m_ui->setupUi(this);
    m_storageGroup->addButton(m_ui->btnLocal, 0);
    m_storageGroup->addButton(m_ui->btnDrive, 1);
    m_ui->btnLocal->setChecked(true);

    m_ui->btnLocal->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));
    m_ui->btnLocal->setIconSize(QSize(32, 32));
    m_ui->btnDrive->setIcon(QIcon(":/icons/application/scalable/actions/google-drive.svg"));
    m_ui->btnDrive->setIconSize(QSize(32, 32));

    auto cardStyle = QStringLiteral(
        "QPushButton {"
        "  border: 2px solid palette(mid);"
        "  border-radius: 8px;"
        "  padding: 12px 16px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:checked {"
        "  border-color: palette(highlight);"
        "  background-color: palette(highlight);"
        "  color: palette(highlighted-text);"
        "}"
        "QPushButton:hover:!checked {"
        "  background-color: palette(light);"
        "}"
    );
    m_ui->btnLocal->setStyleSheet(cardStyle);
    m_ui->btnDrive->setStyleSheet(cardStyle);

    connect(m_storageGroup, &QButtonGroup::idClicked, this, [this](int) {
        updateCardStyles();
    });
}

void DatabaseSettingWidgetMetaData::updateCardStyles()
{
}

bool DatabaseSettingWidgetMetaData::isDriveSelected() const
{
    return m_ui->btnDrive->isChecked();
}

DatabaseSettingWidgetMetaData::~DatabaseSettingWidgetMetaData() = default;

void DatabaseSettingWidgetMetaData::initialize()
{
    Metadata* meta = m_db->metadata();
    auto name = meta->name();
    m_ui->databaseName->setText(name.isEmpty() ? tr("Passwords") : name);
    m_ui->databaseDescription->setText(meta->description());

    m_ui->databaseName->setFocus();
    m_ui->databaseName->selectAll();
}

void DatabaseSettingWidgetMetaData::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_ui->databaseName->setFocus();
}

void DatabaseSettingWidgetMetaData::uninitialize()
{
}

bool DatabaseSettingWidgetMetaData::saveSettings()
{
    Metadata* meta = m_db->metadata();
    meta->setName(m_ui->databaseName->text());
    meta->setDescription(m_ui->databaseDescription->text());

    return true;
}
