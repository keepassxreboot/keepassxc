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

#include "KeyComponentWidget.h"
#include "ui_KeyComponentWidget.h"

#include <QStackedWidget>
#include <QTimer>

KeyComponentWidget::KeyComponentWidget(QWidget* parent)
    : KeyComponentWidget({}, parent)
{
}

KeyComponentWidget::KeyComponentWidget(const QString& name, QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::KeyComponentWidget())
{
    m_ui->setupUi(this);

    connect(m_ui->addButton, SIGNAL(clicked(bool)), SIGNAL(componentAddRequested()));
    connect(m_ui->changeButton, SIGNAL(clicked(bool)), SIGNAL(componentEditRequested()));
    connect(m_ui->removeButton, SIGNAL(clicked(bool)), SIGNAL(componentRemovalRequested()));
    connect(m_ui->cancelButton, SIGNAL(clicked(bool)), SLOT(cancelEdit()));

    connect(m_ui->stackedWidget, SIGNAL(currentChanged(int)), SLOT(resetComponentEditWidget()));

    connect(this, SIGNAL(nameChanged(QString)), SLOT(updateComponentName(QString)));
    connect(this, SIGNAL(descriptionChanged(QString)), SLOT(updateComponentDescription(QString)));
    connect(this, SIGNAL(componentAddRequested()), SLOT(doAdd()));
    connect(this, SIGNAL(componentEditRequested()), SLOT(doEdit()));
    connect(this, SIGNAL(componentRemovalRequested()), SLOT(doRemove()));
    connect(this, SIGNAL(componentAddChanged(bool)), SLOT(updateAddStatus(bool)));

    bool prev = blockSignals(true);
    setComponentName(name);
    blockSignals(prev);

    prev = m_ui->stackedWidget->blockSignals(true);
    m_ui->stackedWidget->setCurrentIndex(Page::AddNew);
    m_ui->stackedWidget->blockSignals(prev);
}

KeyComponentWidget::~KeyComponentWidget()
{
}

/**
 * @param name display name for the key component
 */
void KeyComponentWidget::setComponentName(const QString& name)
{
    if (name == m_componentName) {
        return;
    }

    m_componentName = name;
    emit nameChanged(name);
}

/**
 * @return The key component's display name
 */
QString KeyComponentWidget::componentName() const
{
    return m_componentName;
}

void KeyComponentWidget::setComponentDescription(const QString& description)
{
    if (description == m_componentDescription) {
        return;
    }

    m_componentDescription = description;
    emit descriptionChanged(description);
}

QString KeyComponentWidget::componentDescription() const
{
    return m_componentDescription;
}

void KeyComponentWidget::setComponentAdded(bool added)
{
    if (m_isComponentAdded == added) {
        return;
    }

    m_isComponentAdded = added;
    emit componentAddChanged(added);
}

bool KeyComponentWidget::componentAdded() const
{
    return m_isComponentAdded;
}

void KeyComponentWidget::changeVisiblePage(KeyComponentWidget::Page page)
{
    m_previousPage = static_cast<Page>(m_ui->stackedWidget->currentIndex());
    m_ui->stackedWidget->setCurrentIndex(page);
}

KeyComponentWidget::Page KeyComponentWidget::visiblePage() const
{
    return static_cast<Page>(m_ui->stackedWidget->currentIndex());
}

void KeyComponentWidget::updateComponentName(const QString& name)
{
    m_ui->groupBox->setTitle(name);
    m_ui->addButton->setText(tr("Add %1", "Add a key component").arg(name));
    m_ui->changeButton->setText(tr("Change %1", "Change a key component").arg(name));
    m_ui->removeButton->setText(tr("Remove %1", "Remove a key component").arg(name));
    m_ui->changeOrRemoveLabel->setText(
        tr("%1 set, click to change or remove", "Change or remove a key component").arg(name));
}

void KeyComponentWidget::updateComponentDescription(const QString& description)
{
    m_ui->componentDescription->setText(description);
}

void KeyComponentWidget::updateAddStatus(bool added)
{
    if (added) {
        m_ui->stackedWidget->setCurrentIndex(Page::LeaveOrRemove);
    } else {
        m_ui->stackedWidget->setCurrentIndex(Page::AddNew);
    }
}

void KeyComponentWidget::doAdd()
{
    changeVisiblePage(Page::Edit);
}

void KeyComponentWidget::doEdit()
{
    changeVisiblePage(Page::Edit);
}

void KeyComponentWidget::doRemove()
{
    changeVisiblePage(Page::AddNew);
}

void KeyComponentWidget::cancelEdit()
{
    m_ui->stackedWidget->setCurrentIndex(m_previousPage);
    emit editCanceled();
}

void KeyComponentWidget::showEvent(QShowEvent* event)
{
    resetComponentEditWidget();
    QWidget::showEvent(event);
}

void KeyComponentWidget::resetComponentEditWidget()
{
    if (!m_componentWidget || static_cast<Page>(m_ui->stackedWidget->currentIndex()) == Page::Edit) {
        if (m_componentWidget) {
            delete m_componentWidget;
        }

        m_componentWidget = componentEditWidget();
        m_ui->componentWidgetLayout->addWidget(m_componentWidget);
        initComponentEditWidget(m_componentWidget);
    }

    QTimer::singleShot(0, this, SLOT(updateSize()));
}

void KeyComponentWidget::updateSize()
{
    for (int i = 0; i < m_ui->stackedWidget->count(); ++i) {
        if (m_ui->stackedWidget->currentIndex() == i) {
            m_ui->stackedWidget->widget(i)->setSizePolicy(
                m_ui->stackedWidget->widget(i)->sizePolicy().horizontalPolicy(), QSizePolicy::Preferred);
        } else {
            m_ui->stackedWidget->widget(i)->setSizePolicy(
                m_ui->stackedWidget->widget(i)->sizePolicy().horizontalPolicy(), QSizePolicy::Ignored);
        }
    }
}
