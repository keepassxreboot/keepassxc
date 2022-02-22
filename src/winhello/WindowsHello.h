/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_WINDOWSHELLO_H
#define KEEPASSXC_WINDOWSHELLO_H

#include <QHash>
#include <QObject>

class WindowsHello : public QObject
{
    Q_OBJECT

public:
    static WindowsHello* instance();
    bool isAvailable() const;
    QString errorString() const;
    void reset();

    bool storeKey(const QString& dbPath, const QByteArray& key);
    bool getKey(const QString& dbPath, QByteArray& key);
    bool hasKey(const QString& dbPath) const;
    void reset(const QString& dbPath);

signals:
    void availableChanged(bool state);

private:
    bool m_available = false;
    QString m_error;
    QHash<QString, QByteArray> m_encryptedKeys;

    static WindowsHello* m_instance;
    WindowsHello(QObject* parent = nullptr);
    ~WindowsHello() override = default;
    Q_DISABLE_COPY(WindowsHello);
};

inline WindowsHello* getWindowsHello()
{
    return WindowsHello::instance();
}

#endif // KEEPASSXC_WINDOWSHELLO_H
