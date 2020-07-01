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

#ifndef KEEPASSXC_KEYFILEEDITWIDGET_H
#define KEEPASSXC_KEYFILEEDITWIDGET_H

#include "KeyComponentWidget.h"
#include <QPointer>

namespace Ui
{
    class KeyFileEditWidget;
}

class DatabaseSettingsWidget;

class KeyFileEditWidget : public KeyComponentWidget
{
    Q_OBJECT

public:
    explicit KeyFileEditWidget(DatabaseSettingsWidget* parent);
    Q_DISABLE_COPY(KeyFileEditWidget);
    ~KeyFileEditWidget() override;

    bool addToCompositeKey(QSharedPointer<CompositeKey> key) override;
    bool validate(QString& errorMessage) const override;

protected:
    QWidget* componentEditWidget() override;
    void initComponentEditWidget(QWidget* widget) override;

private slots:
    void createKeyFile();
    void browseKeyFile();

private:
    const QScopedPointer<Ui::KeyFileEditWidget> m_compUi;
    QPointer<QWidget> m_compEditWidget;
    const QPointer<DatabaseSettingsWidget> m_parent;
};

#endif // KEEPASSXC_KEYFILEEDITWIDGET_H
