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

    /**
     * Run a given task then call the defined callback. Prevents event loop blocking and
     * ensures the validity of the follow-on task through the context. If the context is
     * deleted, the callback will not be processed preventing use after free errors.
     *
     * @param task std::function object to run
     * @param context QObject responsible for calling this function
     * @param callback std::function object to run after the task completess
     */
    template <typename FunctionObject, typename FunctionObject2>
    void runThenCallback(FunctionObject task, QObject* context, FunctionObject2 callback)
    {
        typedef QFutureWatcher<typename std::result_of<FunctionObject()>::type> FutureWatcher;
        auto future = QtConcurrent::run(task);
        auto watcher = new FutureWatcher(context);
        QObject::connect(watcher, &QFutureWatcherBase::finished, context, [=]() {
            watcher->deleteLater();
            callback(future.result());
        });
        watcher->setFuture(future);
    }

}; // namespace AsyncTask

#endif // KEEPASSXC_ASYNCTASK_HPP
