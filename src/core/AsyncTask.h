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

#ifndef KEEPASSXC_ASYNCTASK_HPP
#define KEEPASSXC_ASYNCTASK_HPP

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

/**
 * Asynchronously run computations outside the GUI thread.
 */
namespace AsyncTask
{

    /**
     * Wait for the given future without blocking the event loop.
     *
     * @param future future to wait for
     * @return async task result
     */
    template <typename FunctionObject>
    typename std::result_of<FunctionObject()>::type
    waitForFuture(QFuture<typename std::result_of<FunctionObject()>::type> future)
    {
        QEventLoop loop;
        QFutureWatcher<typename std::result_of<FunctionObject()>::type> watcher;
        QObject::connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));
        watcher.setFuture(future);
        loop.exec();
        return future.result();
    }

    /**
     * Run a given task and wait for it to finish without blocking the event loop.
     *
     * @param task std::function object to run
     * @return async task result
     */
    template <typename FunctionObject>
    typename std::result_of<FunctionObject()>::type runAndWaitForFuture(FunctionObject task)
    {
        return waitForFuture<FunctionObject>(QtConcurrent::run(task));
    }

}; // namespace AsyncTask

#endif // KEEPASSXC_ASYNCTASK_HPP
