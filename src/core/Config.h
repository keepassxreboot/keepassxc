/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_CONFIG_H
#define KEEPASSX_CONFIG_H

#include <QScopedPointer>
#include <QVariant>

class QSettings;

class Config : public QObject
{
Q_OBJECT

public:
    Q_DISABLE_COPY(Config)

    ~Config() override;
    QVariant get(const QString& key);
    QVariant get(const QString& key, const QVariant& defaultValue);
    QString getFileName();
    void set(const QString& key, const QVariant& value);
    bool hasAccessError();
    void sync();

    static Config* instance();
    static void createConfigFromFile(const QString& file);
    static void createTempFileInstance();

private:
    Config(const QString& fileName, QObject* parent);
    explicit Config(QObject* parent);
    void init(const QString& fileName);

    static Config* m_instance;

    QScopedPointer<QSettings> m_settings;
    QHash<QString, QVariant> m_defaults;
};

inline Config* config() {
    return Config::instance();
}

#endif // KEEPASSX_CONFIG_H
