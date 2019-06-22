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

#include "PasswordEditWidget.h"
#include "ui_PasswordEditWidget.h"

#include "core/FilePath.h"
#include "gui/PasswordGeneratorWidget.h"
#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

#include <QDialog>

PasswordEditWidget::PasswordEditWidget(QWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::PasswordEditWidget())
{
    setComponentName(tr("Password"));
    setComponentDescription(tr("<p>A password is the primary method for securing your database.</p>"
                               "<p>Good passwords are long and unique. KeePassXC can generate one for you.</p>"));
}

PasswordEditWidget::~PasswordEditWidget()
{
}

bool PasswordEditWidget::addToCompositeKey(QSharedPointer<CompositeKey> key)
{
    QString pw = m_compUi->enterPasswordEdit->text();
    if (!pw.isEmpty()) {
        key->addKey(QSharedPointer<PasswordKey>::create(pw));
        return true;
    }
    return false;
}

/**
 * @param visible changed password visibility state
 */
void PasswordEditWidget::setPasswordVisible(bool visible)
{
    m_compUi->togglePasswordButton->setChecked(visible);
}

/**
 * @return password visibility state
 */
bool PasswordEditWidget::isPasswordVisible() const
{
    return m_compUi->togglePasswordButton->isChecked();
}

bool PasswordEditWidget::isEmpty() const
{
    return (visiblePage() == Page::Edit) && m_compUi->enterPasswordEdit->text().isEmpty();
}

QWidget* PasswordEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);
    m_compUi->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    m_compUi->passwordGeneratorButton->setIcon(filePath()->icon("actions", "password-generator", false));
    m_compUi->repeatPasswordEdit->enableVerifyMode(m_compUi->enterPasswordEdit);

    connect(m_compUi->togglePasswordButton,
            SIGNAL(toggled(bool)),
            m_compUi->enterPasswordEdit,
            SLOT(setShowPassword(bool)));
    connect(m_compUi->passwordGeneratorButton, SIGNAL(clicked(bool)), SLOT(showPasswordGenerator()));

    return m_compEditWidget;
}

void PasswordEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
    m_compUi->enterPasswordEdit->setFocus();
}

void PasswordEditWidget::hideEvent(QHideEvent* event)
{
    Q_ASSERT(m_compUi->enterPasswordEdit);

    if (!isVisible() && m_compUi->enterPasswordEdit) {
        m_compUi->enterPasswordEdit->setText("");
        m_compUi->repeatPasswordEdit->setText("");
    }

    QWidget::hideEvent(event);
}

bool PasswordEditWidget::validate(QString& errorMessage) const
{
    if (m_compUi->enterPasswordEdit->text() != m_compUi->repeatPasswordEdit->text()) {
        errorMessage = tr("Passwords do not match.");
        return false;
    }

    return true;
}

void PasswordEditWidget::showPasswordGenerator()
{
    QDialog pwDialog;
    pwDialog.setWindowTitle(tr("Generate master password"));

    auto layout = new QVBoxLayout();
    pwDialog.setLayout(layout);

    auto pwGenerator = new PasswordGeneratorWidget(&pwDialog);
    layout->addWidget(pwGenerator);

    pwGenerator->setStandaloneMode(false);
    connect(pwGenerator, SIGNAL(appliedPassword(QString)), SLOT(setPassword(QString)));
    connect(pwGenerator, SIGNAL(dialogTerminated()), &pwDialog, SLOT(close()));

    pwGenerator->setPasswordVisible(isPasswordVisible());

    pwDialog.exec();
}

void PasswordEditWidget::setPassword(const QString& password)
{
    Q_ASSERT(m_compEditWidget);

    m_compUi->enterPasswordEdit->setText(password);
    m_compUi->repeatPasswordEdit->setText(password);
}
