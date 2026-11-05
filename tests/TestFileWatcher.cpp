/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "TestFileWatcher.h"

#include "core/FileWatcher.h"

#include <QSemaphore>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>
#include <QtConcurrent>

QTEST_GUILESS_MAIN(TestFileWatcher)

namespace
{
    // Keeps every global pool thread busy until released, so new tasks wait in the queue
    class PoolBlocker
    {
    public:
        PoolBlocker()
        {
            for (int i = 0; i < m_count; ++i) {
                auto future = QtConcurrent::run([this] {
                    m_busy.release();
                    m_gate.acquire();
                });
                Q_UNUSED(future)
            }
            m_busy.acquire(m_count);
        }

        void release()
        {
            m_gate.release(m_count);
            QThreadPool::globalInstance()->waitForDone(5000);
        }

    private:
        const int m_count = QThreadPool::globalInstance()->maxThreadCount();
        QSemaphore m_gate;
        QSemaphore m_busy;
    };
} // namespace

void TestFileWatcher::testDeleteDuringChecksum()
{
    // The periodic change check computes the checksum on a worker thread. Deleting the
    // watcher before that task runs must not leave the task reading the deleted object.
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.write(QByteArray(4096, 'x')) > 0);
    file.flush();

    auto watcher = new FileWatcher();
    watcher->start(file.fileName());

    PoolBlocker blocker;
    QVERIFY(QMetaObject::invokeMethod(watcher, "checkFileChanged"));
    delete watcher;

    blocker.release();
    QCoreApplication::processEvents();
}

void TestFileWatcher::testRestartSameFileDuringChecksum()
{
    // Saving the database restarts the watcher on the same file. A checksum of the old
    // content that arrives after the restart must not be reported as an external change.
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.write("old") > 0);
    file.flush();

    FileWatcher watcher;
    QSignalSpy spy(&watcher, &FileWatcher::fileChanged);
    watcher.start(file.fileName());

    PoolBlocker blocker;
    QVERIFY(QMetaObject::invokeMethod(&watcher, "checkFileChanged"));
    // Let the task hash the old content; its result waits in the event queue
    blocker.release();

    QVERIFY(file.seek(0));
    QVERIFY(file.write("new") > 0);
    file.flush();
    watcher.start(file.fileName());

    QTest::qWait(100);
    QCOMPARE(spy.count(), 0);
}

void TestFileWatcher::testUnreadableFile()
{
    // A file that can't be read, e.g. on a network share that is briefly gone, is not a change
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.write("content") > 0);
    file.flush();

    FileWatcher watcher;
    QSignalSpy spy(&watcher, &FileWatcher::fileChanged);
    watcher.start(file.fileName());

    // Closes the file first, Windows can't delete a file that is still open
    QVERIFY(file.remove());
    QVERIFY(QMetaObject::invokeMethod(&watcher, "checkFileChanged"));

    QTest::qWait(100);
    QCOMPARE(spy.count(), 0);
}
