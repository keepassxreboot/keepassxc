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
#include "ui_KeyComponentWidget.h"
#include "ui_PasswordEditWidget.h"

#include "keys/CompositeKey.h"
#include "keys/PasswordKey.h"

PasswordEditWidget::PasswordEditWidget(QWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::PasswordEditWidget())
{
    initComponent();

    // Explicitly clear password on cancel
    connect(this, &PasswordEditWidget::editCanceled, this, [this] { setPassword({}); });
}

PasswordEditWidget::~PasswordEditWidget() = default;

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
    m_compUi->enterPasswordEdit->setShowPassword(visible);
}

/**
 * @return password visibility state
 */
bool PasswordEditWidget::isPasswordVisible() const
{
    return m_compUi->enterPasswordEdit->isPasswordVisible();
}

bool PasswordEditWidget::isEmpty() const
{
    return visiblePage() != Page::Edit || m_compUi->enterPasswordEdit->text().isEmpty();
}

PasswordHealth::Quality PasswordEditWidget::getPasswordQuality() const
{
    QString pwd = m_compUi->enterPasswordEdit->text();
    PasswordHealth passwordHealth(pwd);

    return passwordHealth.quality();
}

QWidget* PasswordEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);
    m_compUi->enterPasswordEdit->enablePasswordGenerator();
    m_compUi->enterPasswordEdit->setRepeatPartner(m_compUi->repeatPasswordEdit);
    return m_compEditWidget;
}

void PasswordEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
    setFocusProxy(m_compUi->enterPasswordEdit);
    m_compUi->enterPasswordEdit->setQualityVisible(true);
    m_compUi->repeatPasswordEdit->setQualityVisible(false);
}

void PasswordEditWidget::initComponent()
{
    // These need to be set in total for each credential type for translation purposes
    m_ui->groupBox->setTitle(tr("Password"));
    m_ui->addButton->setText(tr("Add Password"));
    m_ui->changeButton->setText(tr("Change Password"));
    m_ui->removeButton->setText(tr("Remove Password"));
    m_ui->changeOrRemoveLabel->setText(tr("Password set, click to change or remove"));

    m_ui->componentDescription->setText(
        tr("<p>A password is the primary method for securing your database.</p>"
           "<p>Good passwords are long and unique. KeePassXC can generate one for you.</p>"));
}

bool PasswordEditWidget::validate(QString& errorMessage) const
{
    if (m_compUi->enterPasswordEdit->text() != m_compUi->repeatPasswordEdit->text()) {
        errorMessage = tr("Passwords do not match.");
        return false;
    }

    return true;
}

void PasswordEditWidget::setPassword(const QString& password)
{
    Q_ASSERT(m_compEditWidget);

    m_compUi->enterPasswordEdit->setText(password);
    m_compUi->repeatPasswordEdit->setText(password);
}
