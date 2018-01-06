/*
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

#ifndef KEEPASSX_KEEPASS2WRITER_H
#define KEEPASSX_KEEPASS2WRITER_H

#include <QIODevice>
#include <QByteArray>
#include <QString>
#include <QCoreApplication>
#include <QScopedPointer>

#include "core/Database.h"
#include "format/KeePass2.h"

#define CHECK_RETURN_FALSE(x) if (!(x)) return false;

class BaseKeePass2Writer
{
public:
    BaseKeePass2Writer();

    virtual bool writeDatabase(QIODevice* device, Database* db) = 0;
    virtual bool writeDatabase(const QString& filename, Database* db);

    virtual bool hasError();
    virtual QString errorString();

    virtual ~BaseKeePass2Writer();

protected:
    void raiseError(const QString& errorMessage);

    bool m_error;
    QString m_errorStr;
};

class KeePass2Writer : public BaseKeePass2Writer
{
    Q_DECLARE_TR_FUNCTIONS(KeePass2Writer)

public:
    virtual bool writeDatabase(QIODevice* device, Database* db) override;
    using BaseKeePass2Writer::writeDatabase;

    virtual bool hasError() override;
    virtual QString errorString() override;

private:
    QScopedPointer<BaseKeePass2Writer> m_writer;
};

#endif // KEEPASSX_KEEPASS2READER_H
