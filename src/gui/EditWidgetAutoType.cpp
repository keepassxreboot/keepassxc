/*
 *  Copyright (C) 2017 Serhii Moroz <frostasm@gmail.com>
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

#include "EditWidgetAutoType.h"
#include "ui_EditWidgetAutoType.h"

#include <QButtonGroup>

#include "core/Tools.h"
#include "core/AutoTypeAssociations.h"
#include "gui/AutoTypeAssociationsModel.h"


EditWidgetAutoType::EditWidgetAutoType(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::EditWidgetAutoType())
    , m_history(false)
    , m_parentAutoTypeEnabled(false)
    , m_autoTypeAssoc(new AutoTypeAssociations(this))
    , m_autoTypeAssocModel(new AutoTypeAssociationsModel(this))
    , m_defaultSequenceGroup(new QButtonGroup(this))
    , m_windowSequenceGroup(new QButtonGroup(this))
{
    m_ui->setupUi(this);

    m_defaultSequenceGroup->addButton(m_ui->inheritSequenceButton);
    m_defaultSequenceGroup->addButton(m_ui->customSequenceButton);
    m_windowSequenceGroup->addButton(m_ui->defaultWindowSequenceButton);
    m_windowSequenceGroup->addButton(m_ui->customWindowSequenceButton);
    m_autoTypeAssocModel->setAutoTypeAssociations(m_autoTypeAssoc);
    m_ui->assocView->setModel(m_autoTypeAssocModel);
    m_ui->assocView->setColumnHidden(1, true);

    connect(m_ui->enableComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateAutoTypeEnabled()));
    connect(m_ui->customSequenceButton, SIGNAL(toggled(bool)),
            m_ui->sequenceEdit, SLOT(setEnabled(bool)));
    connect(m_ui->customWindowSequenceButton, SIGNAL(toggled(bool)),
            m_ui->windowSequenceEdit, SLOT(setEnabled(bool)));
    connect(m_ui->assocAddButton, SIGNAL(clicked()), SLOT(insertAutoTypeAssoc()));
    connect(m_ui->assocRemoveButton, SIGNAL(clicked()), SLOT(removeAutoTypeAssoc()));
    connect(m_ui->assocView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(updateAutoTypeEnabled()));
    connect(m_autoTypeAssocModel, SIGNAL(modelReset()), SLOT(updateAutoTypeEnabled()));
    connect(m_ui->assocView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(loadCurrentAssoc(QModelIndex)));
    connect(m_autoTypeAssocModel, SIGNAL(modelReset()), SLOT(clearCurrentAssoc()));
    connect(m_ui->windowTitleCombo, SIGNAL(editTextChanged(QString)),
            SLOT(applyCurrentAssoc()));
    connect(m_ui->defaultWindowSequenceButton, SIGNAL(toggled(bool)),
            SLOT(applyCurrentAssoc()));
    connect(m_ui->windowSequenceEdit, SIGNAL(textChanged(QString)),
            SLOT(applyCurrentAssoc()));
}

EditWidgetAutoType::~EditWidgetAutoType()
{

}

Tools::TriState EditWidgetAutoType::autoTypeEnabled() const
{
    return m_ui->enableComboBox->triState();
}

bool EditWidgetAutoType::inheritSequenceEnabled() const
{
    return m_ui->inheritSequenceButton->isChecked();
}

QString EditWidgetAutoType::sequence() const
{
    return m_ui->sequenceEdit->text();
}

AutoTypeAssociations *EditWidgetAutoType::autoTypeAssociations()
{
    return m_autoTypeAssoc;
}

const AutoTypeAssociations *EditWidgetAutoType::autoTypeAssociations() const
{
    return m_autoTypeAssoc;
}

void EditWidgetAutoType::setFields(const Tools::TriState autoTypeEnabled, const bool parentAutoTypeEnabled,
                                   const QString &defaultAutoTypeSequence, const QString &effectiveAutoTypeSequence,
                                   const AutoTypeAssociations* autoTypeAssociations)
{    
    Q_ASSERT(autoTypeAssociations);

    m_parentAutoTypeEnabled = parentAutoTypeEnabled;
    m_ui->enableComboBox->addTriStateItems(parentAutoTypeEnabled);
    m_ui->enableComboBox->setTriState(autoTypeEnabled);
    if (defaultAutoTypeSequence.isEmpty()) {
        m_ui->inheritSequenceButton->setChecked(true);
    }
    else {
        m_ui->customSequenceButton->setChecked(true);
    }
    m_ui->sequenceEdit->setText(effectiveAutoTypeSequence);
    m_ui->windowTitleCombo->lineEdit()->clear();
    m_ui->defaultWindowSequenceButton->setChecked(true);
    m_ui->windowSequenceEdit->setText("");
    m_autoTypeAssoc->copyDataFrom(autoTypeAssociations);

    if (m_autoTypeAssoc->size() != 0) {
        m_ui->assocView->setCurrentIndex(m_autoTypeAssocModel->index(0, 0));
    }
    if (!m_history) {
        m_ui->windowTitleCombo->refreshWindowList();
    }
    updateAutoTypeEnabled();
}

void EditWidgetAutoType::setHistory(bool history)
{
    m_history = history;
    m_ui->sequenceEdit->setReadOnly(history);
    m_ui->windowTitleCombo->lineEdit()->setReadOnly(history);
    m_ui->windowSequenceEdit->setReadOnly(history);
}

void EditWidgetAutoType::clear()
{
    m_autoTypeAssoc->clear();
}

void EditWidgetAutoType::removeEmptyAssocs()
{
    m_autoTypeAssoc->removeEmpty();
}

void EditWidgetAutoType::updateAutoTypeEnabled()
{
    if (m_ui->enableComboBox->currentIndex() < 0) // omit invalid values
        return;

    bool autoTypeEnabled = Tools::isTriStateEnabled(m_ui->enableComboBox->triState(), m_parentAutoTypeEnabled);
    bool validIndex = m_ui->assocView->currentIndex().isValid() && m_autoTypeAssoc->size() != 0;

    m_ui->enableComboBox->setEnabled(!m_history);
    m_ui->inheritSequenceButton->setEnabled(!m_history && autoTypeEnabled);
    m_ui->customSequenceButton->setEnabled(!m_history && autoTypeEnabled);
    m_ui->sequenceEdit->setEnabled(autoTypeEnabled && m_ui->customSequenceButton->isChecked());

    m_ui->assocView->setEnabled(autoTypeEnabled);
    m_ui->assocAddButton->setEnabled(!m_history);
    m_ui->assocRemoveButton->setEnabled(!m_history && validIndex);

    m_ui->windowTitleLabel->setEnabled(autoTypeEnabled && validIndex);
    m_ui->windowTitleCombo->setEnabled(autoTypeEnabled && validIndex);
    m_ui->defaultWindowSequenceButton->setEnabled(!m_history && autoTypeEnabled && validIndex);
    m_ui->customWindowSequenceButton->setEnabled(!m_history && autoTypeEnabled && validIndex);
    m_ui->windowSequenceEdit->setEnabled(autoTypeEnabled && validIndex
                                                 && m_ui->customWindowSequenceButton->isChecked());
}

void EditWidgetAutoType::insertAutoTypeAssoc()
{
    AutoTypeAssociations::Association assoc;
    m_autoTypeAssoc->add(assoc);
    QModelIndex newIndex = m_autoTypeAssocModel->index(m_autoTypeAssoc->size() - 1, 0);
    m_ui->assocView->setCurrentIndex(newIndex);
    loadCurrentAssoc(newIndex);
    m_ui->windowTitleCombo->setFocus();
}

void EditWidgetAutoType::removeAutoTypeAssoc()
{
    QModelIndex currentIndex = m_ui->assocView->currentIndex();

    if (currentIndex.isValid()) {
        m_autoTypeAssoc->remove(currentIndex.row());
    }
}

void EditWidgetAutoType::loadCurrentAssoc(const QModelIndex& current)
{
    if (current.isValid() && current.row() < m_autoTypeAssoc->size()) {
        AutoTypeAssociations::Association assoc = m_autoTypeAssoc->get(current.row());
        m_ui->windowTitleCombo->setEditText(assoc.window);
        if (assoc.sequence.isEmpty()) {
            m_ui->defaultWindowSequenceButton->setChecked(true);
        }
        else {
            m_ui->customWindowSequenceButton->setChecked(true);
        }
        m_ui->windowSequenceEdit->setText(assoc.sequence);

        updateAutoTypeEnabled();
    }
    else {
        clearCurrentAssoc();
    }
}

void EditWidgetAutoType::clearCurrentAssoc()
{
    m_ui->windowTitleCombo->setEditText("");

    m_ui->defaultWindowSequenceButton->setChecked(true);
    m_ui->windowSequenceEdit->setText("");

    updateAutoTypeEnabled();
}

void EditWidgetAutoType::applyCurrentAssoc()
{
    QModelIndex index = m_ui->assocView->currentIndex();

    if (!index.isValid() || m_autoTypeAssoc->size() == 0 || m_history) {
        return;
    }

    AutoTypeAssociations::Association assoc;
    assoc.window = m_ui->windowTitleCombo->currentText();
    if (m_ui->customWindowSequenceButton->isChecked()) {
        assoc.sequence = m_ui->windowSequenceEdit->text();
    }

    m_autoTypeAssoc->update(index.row(), assoc);
}
