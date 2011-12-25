/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
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
  ModelTest( QAbstractItemModel *model, QObject *parent = 0 );

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
  void rowsAboutToBeInserted( const QModelIndex &parent, int start, int end );
  void rowsInserted( const QModelIndex & parent, int start, int end );
  void rowsAboutToBeRemoved( const QModelIndex &parent, int start, int end );
  void rowsRemoved( const QModelIndex & parent, int start, int end );

private:
  void checkChildren( const QModelIndex &parent, int currentDepth = 0 );

  QAbstractItemModel *model;

  struct Changing {
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
