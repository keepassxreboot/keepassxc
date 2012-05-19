/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "DatabaseSettingsWidget.h"
#include "ui_DatabaseSettingsWidget.h"

#include "core/Database.h"
#include "core/Metadata.h"
#include "keys/CompositeKey.h"

DatabaseSettingsWidget::DatabaseSettingsWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidget())
    , m_db(0)
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(save()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_ui->historyMaxItemsCheckBox, SIGNAL(toggled(bool)),
            m_ui->historyMaxItemsSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->historyMaxSizeCheckBox, SIGNAL(toggled(bool)),
            m_ui->historyMaxSizeSpinBox, SLOT(setEnabled(bool)));
    connect(m_ui->transformBenchmarkButton, SIGNAL(clicked()), SLOT(transformRoundsBenchmark()));
}

DatabaseSettingsWidget::~DatabaseSettingsWidget()
{
}

void DatabaseSettingsWidget::load(Database* db)
{
    m_db = db;

    Metadata* meta = m_db->metadata();

    m_ui->dbNameEdit->setText(meta->name());
    m_ui->dbDescriptionEdit->setText(meta->description());
    m_ui->recycleBinEnabledCheckBox->setChecked(meta->recycleBinEnabled());
    m_ui->defaultUsernameEdit->setText(meta->defaultUserName());
    m_ui->transformRoundsSpinBox->setValue(m_db->transformRounds());
    if (meta->historyMaxItems() > -1) {
        m_ui->historyMaxItemsSpinBox->setValue(meta->historyMaxItems());
        m_ui->historyMaxItemsCheckBox->setChecked(true);
    }
    else {
        m_ui->historyMaxItemsSpinBox->setValue(Metadata::DefaultHistoryMaxItems);
        m_ui->historyMaxItemsCheckBox->setChecked(false);
    }
    if (meta->historyMaxSize() > -1) {
        m_ui->historyMaxSizeSpinBox->setValue(meta->historyMaxSize());
        m_ui->historyMaxSizeCheckBox->setChecked(true);
    }
    else {
        m_ui->historyMaxSizeSpinBox->setValue(Metadata::DefaultHistoryMaxSize);
        m_ui->historyMaxSizeCheckBox->setChecked(false);
    }

    m_ui->dbNameEdit->setFocus();
}

void DatabaseSettingsWidget::save()
{
    Metadata* meta = m_db->metadata();

    meta->setName(m_ui->dbNameEdit->text());
    meta->setDescription(m_ui->dbDescriptionEdit->text());
    meta->setDefaultUserName(m_ui->defaultUsernameEdit->text());
    meta->setRecycleBinEnabled(m_ui->recycleBinEnabledCheckBox->isChecked());
    m_db->setTransformRounds(m_ui->transformRoundsSpinBox->value());

    if (m_ui->historyMaxItemsCheckBox->isChecked()) {
        meta->setHistoryMaxItems(m_ui->historyMaxItemsSpinBox->value());
    }
    else {
        meta->setHistoryMaxItems(-1);
    }
    if (m_ui->historyMaxSizeCheckBox->isChecked()) {
        meta->setHistoryMaxSize(m_ui->historyMaxSizeSpinBox->value());
    }
    else {
        meta->setHistoryMaxSize(-1);
    }

    Q_EMIT editFinished(true);
}

void DatabaseSettingsWidget::reject()
{
    Q_EMIT editFinished(false);
}

void DatabaseSettingsWidget::transformRoundsBenchmark()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_ui->transformRoundsSpinBox->setValue(CompositeKey::transformKeyBenchmark(1000));
    QApplication::restoreOverrideCursor();
}
