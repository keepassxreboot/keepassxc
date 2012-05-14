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

#include "core/Metadata.h"
#include "keys/CompositeKey.h"

DatabaseSettingsWidget::DatabaseSettingsWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseSettingsWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(changeSettings()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_ui->historyMaxItemsCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(toggleHistoryMaxItemsSpinBox(int)));
    connect(m_ui->historyMaxSizeCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(toggleHistoryMaxSizeSpinBox(int)));
    connect(m_ui->transformBenchmarkButton, SIGNAL(clicked()), SLOT(transformRoundsBenchmark()));
}

DatabaseSettingsWidget::~DatabaseSettingsWidget()
{
}

void DatabaseSettingsWidget::setForms(const QString& dbName, const QString& dbDescription,
                                      const QString& defaultUsername, bool recylceBinEnabled,
                                      int transformRounds, int historyMaxItems,
                                      int historyMaxSize)
{
    m_ui->dbNameEdit->setText(dbName);
    m_ui->dbDescriptionEdit->setText(dbDescription);
    if (recylceBinEnabled) {
        m_ui->recycleBinEnabledCheckBox->setCheckState(Qt::Checked);
    }
    else {
        m_ui->recycleBinEnabledCheckBox->setCheckState(Qt::Unchecked);
    }
    m_ui->defaultUsernameEdit->setText(defaultUsername);
    m_ui->transformRoundsSpinBox->setValue(transformRounds);
    if (historyMaxItems > -1) {
        m_ui->historyMaxItemsSpinBox->setValue(historyMaxItems);
        m_ui->historyMaxItemsCheckBox->setChecked(true);
    }
    else {
        m_ui->historyMaxItemsSpinBox->setValue(Metadata::DefaultHistoryMaxItems);
        m_ui->historyMaxItemsCheckBox->setChecked(false);
    }
    if (historyMaxSize > -1) {
        m_ui->historyMaxSizeSpinBox->setValue(historyMaxSize);
        m_ui->historyMaxSizeCheckBox->setChecked(true);
    }
    else {
        m_ui->historyMaxSizeSpinBox->setValue(Metadata::DefaultHistoryMaxSize);
        m_ui->historyMaxSizeCheckBox->setChecked(false);
    }

    m_ui->dbNameEdit->setFocus();
}

quint64 DatabaseSettingsWidget::transformRounds()
{
    return m_transformRounds;
}

QString DatabaseSettingsWidget::dbName()
{
    return m_dbName;
}

QString DatabaseSettingsWidget::dbDescription()
{
    return m_dbDescription;
}

QString DatabaseSettingsWidget::defaultUsername()
{
    return m_defaultUsername;
}

bool DatabaseSettingsWidget::recylceBinEnabled()
{
    return m_recylceBinEnabled;
}

int DatabaseSettingsWidget::historyMaxItems()
{
    return m_historyMaxItems;
}

int DatabaseSettingsWidget::historyMaxSize()
{
    return m_historyMaxSize;
}

void DatabaseSettingsWidget::changeSettings()
{
    m_dbName = m_ui->dbNameEdit->text();
    m_dbDescription = m_ui->dbDescriptionEdit->text();
    m_defaultUsername = m_ui->defaultUsernameEdit->text();
    if (m_ui->recycleBinEnabledCheckBox->checkState() == Qt::Checked) {
        m_recylceBinEnabled = true;
    }
    else {
        m_recylceBinEnabled = false;
    }
    m_transformRounds = m_ui->transformRoundsSpinBox->value();
    if (m_ui->historyMaxItemsCheckBox->isChecked()) {
        m_historyMaxItems = m_ui->historyMaxItemsSpinBox->value();
    }
    else {
        m_historyMaxItems = -1;
    }
    if (m_ui->historyMaxSizeCheckBox->isChecked()) {
        m_historyMaxSize = m_ui->historyMaxSizeSpinBox->value();
    }
    else {
        m_historyMaxSize = -1;
    }


    Q_EMIT editFinished(true);
}

void DatabaseSettingsWidget::reject()
{
    Q_EMIT editFinished(false);
}

void DatabaseSettingsWidget::toggleHistoryMaxItemsSpinBox(int state)
{
    if (state == Qt::Checked) {
        m_ui->historyMaxItemsSpinBox->setEnabled(true);
    }
    else {
        m_ui->historyMaxItemsSpinBox->setEnabled(false);
    }
}

void DatabaseSettingsWidget::toggleHistoryMaxSizeSpinBox(int state)
{
    if (state == Qt::Checked) {
        m_ui->historyMaxSizeSpinBox->setEnabled(true);
    }
    else {
        m_ui->historyMaxSizeSpinBox->setEnabled(false);
    }
}

void DatabaseSettingsWidget::transformRoundsBenchmark()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_ui->transformRoundsSpinBox->setValue(CompositeKey::transformKeyBenchmark(1000));
    QApplication::restoreOverrideCursor();
}
