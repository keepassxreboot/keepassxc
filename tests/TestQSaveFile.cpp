/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#include "TestQSaveFile.h"

#include <QtTest>

#include <unistd.h>

#if defined(Q_OS_WIN)
# include <windows.h>
#endif

#include "tests.h"
#include "core/qsavefile.h"

QTEST_GUILESS_MAIN(TestQSaveFile)

class DirCleanup
{
public:
    DirCleanup(const QString& dir, const QString& filePrefix) : m_dir(dir), m_filePrefix(filePrefix) {}
    ~DirCleanup() {
        QDir dir(m_dir);
        QStringList files = dir.entryList(QStringList() << (m_filePrefix + "*"), QDir::Files);
        Q_FOREACH (const QString& file, files) {
            QFile::remove(m_dir + "/" + file);
        }

        QDir().rmdir(m_dir);
    }

private:
    QString m_dir;
    QString m_filePrefix;
};

void TestQSaveFile::transactionalWrite()
{
    const QString dir = tmpDir();
    QVERIFY(!dir.isEmpty());
    const QString targetFile = dir + QString::fromLatin1("/outfile");
    DirCleanup dirCleanup(dir, "outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.isOpen());
    QCOMPARE(file.fileName(), targetFile);
    QVERIFY(!QFile::exists(targetFile));

    QTextStream ts(&file);
    ts << "This is test data one.\n";
    ts.flush();
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));

    QVERIFY(file.commit());
    QVERIFY(QFile::exists(targetFile));
    QCOMPARE(file.fileName(), targetFile);

    // Check that we can reuse a QSaveFile object
    // (and test the case of an existing target file)
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("Hello"), 5LL);
    QVERIFY(file.commit());

    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll().constData()), QString::fromLatin1("Hello"));
    reader.close();

    QVERIFY(QFile::remove(targetFile));
}

void TestQSaveFile::autoFlush()
{
    const QString dir = tmpDir();
    QVERIFY(!dir.isEmpty());
    const QString targetFile = dir + QString::fromLatin1("/outfile");
    DirCleanup dirCleanup(dir, "outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));

    QTextStream ts(&file);
    ts << "Auto-flush.";
    // no flush
    QVERIFY(file.commit()); // close will emit aboutToClose, which will flush the stream
    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll().constData()), QString::fromLatin1("Auto-flush."));
    reader.close();

    QVERIFY(QFile::remove(targetFile));
}

void TestQSaveFile::transactionalWriteNoPermissions()
{
#ifdef Q_OS_UNIX
    if (::geteuid() == 0) {
        QSKIP("not valid running this test as root", SkipAll);
    }

    // You can write into /dev/zero, but you can't create a /dev/zero.XXXXXX temp file.
    QSaveFile file("/dev/zero");
    if (!QDir("/dev").exists()) {
        QSKIP("/dev doesn't exist on this system", SkipAll);
    }

    QVERIFY(!file.open(QIODevice::WriteOnly));
    QCOMPARE(static_cast<int>(file.error()), static_cast<int>(QFile::OpenError));
    QVERIFY(!file.commit());
#endif
}

void TestQSaveFile::transactionalWriteCanceled()
{
    const QString dir = tmpDir();
    QVERIFY(!dir.isEmpty());
    const QString targetFile = dir + QString::fromLatin1("/outfile");
    DirCleanup dirCleanup(dir, "outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));

    QTextStream ts(&file);
    ts << "This writing operation will soon be canceled.\n";
    ts.flush();
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));

    // We change our mind, let's abort writing
    file.cancelWriting();

    QVERIFY(!file.commit());

    QVERIFY(!QFile::exists(targetFile)); // temp file was discarded
    QCOMPARE(file.fileName(), targetFile);
}

void TestQSaveFile::transactionalWriteErrorRenaming()
{
#ifndef Q_OS_WIN
    if (::geteuid() == 0) {
        QSKIP("not valid running this test as root", SkipAll);
    }
    const QString dir = tmpDir();
    QVERIFY(!dir.isEmpty());
    const QString targetFile = dir + QString::fromLatin1("/outfile");
    DirCleanup dirCleanup(dir, "outfile");
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("Hello"), qint64(5));
    QVERIFY(!QFile::exists(targetFile));

#ifdef Q_OS_UNIX
    QFile dirAsFile(dir); // yay, I have to use QFile to change a dir's permissions...
    QVERIFY(dirAsFile.setPermissions(QFile::Permissions(0))); // no permissions
#else
    QVERIFY(file.setPermissions(QFile::ReadOwner));
#endif

    QVERIFY(!file.commit());
    QVERIFY(!QFile::exists(targetFile)); // renaming failed
    QCOMPARE(file.error(), QFile::RenameError);

    // Restore permissions so that the cleanup can happen
#ifdef Q_OS_UNIX
    QVERIFY(dirAsFile.setPermissions(QFile::Permissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)));
#else
    QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
#endif
#endif // !Q_OS_WIN
}

QString TestQSaveFile::tmpDir()
{
    QTemporaryFile* tmpFile = new QTemporaryFile(QDir::tempPath() + "/qttest_temp.XXXXXX");
    if (!tmpFile->open()) {
        return QString();
    }
    QString dirName = tmpFile->fileName();
    delete tmpFile;
    if (!QDir().mkdir(dirName)) {
        return QString();
    }

    return dirName;
}
