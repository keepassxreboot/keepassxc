/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "core/Global.h"

class QSettings;

class Config : public QObject
{
    Q_OBJECT

public:
    ~Config();
    QVariant get(const QString& key);
    QVariant get(const QString& key, const QVariant& defaultValue);
    void set(const QString& key, const QVariant& value);

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

    Q_DISABLE_COPY(Config)
};

inline Config* config() {
    return Config::instance();
}

#endif // KEEPASSX_CONFIG_H
