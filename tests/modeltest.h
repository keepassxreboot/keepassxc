/****************************************************************************
**
** Copyright (C) 2007 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Concurrent project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL-2 included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef MODELTEST_H
#define MODELTEST_H

#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QStack>

class ModelTest : public QObject
{
    Q_OBJECT

public:
    ModelTest(QAbstractItemModel *model, QObject *parent = 0);

private Q_SLOTS:
    void nonDestructiveBasicTest();
    void rowCount();
    void columnCount();
    void hasIndex();
    void index();
    void parent();
    void data();

protected Q_SLOTS:
    void runAllTests();
    void layoutAboutToBeChanged();
    void layoutChanged();
    void rowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void rowsInserted(const QModelIndex & parent, int start, int end);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex & parent, int start, int end);

private:
    void checkChildren(const QModelIndex &parent, int currentDepth = 0);

    QAbstractItemModel *model;

    struct Changing
    {
        QModelIndex parent;
        int oldSize;
        QVariant last;
        QVariant next;
    };
    QStack<Changing> insert;
    QStack<Changing> remove;

    bool fetchingMore;

    QList<QPersistentModelIndex> changing;
};

#endif
