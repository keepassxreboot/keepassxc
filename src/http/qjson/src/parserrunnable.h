/* This file is part of qjson
  *
  * Copyright (C) 2009 Flavio Castelli <flavio@castelli.name>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License version 2.1, as published by the Free Software Foundation.
  * 
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef PARSERRUNNABLE_H
#define PARSERRUNNABLE_H

#include "qjson_export.h"

#include <QtCore/QObject>
#include <QtCore/QRunnable>

QT_BEGIN_NAMESPACE
class QVariant;
QT_END_NAMESPACE

namespace QJson {
  /**
  * @brief Convenience class for converting JSON data to QVariant objects using a dedicated thread
  */
  class QJSON_EXPORT ParserRunnable  : public QObject, public QRunnable
  {
    Q_OBJECT
    public:
      explicit ParserRunnable(QObject* parent = 0);
      ~ParserRunnable();

      void setData( const QByteArray& data );

      void run();

    Q_SIGNALS:
      /**
      * This signal is emitted when the parsing process has been completed
      * @param json contains the result of the parsing
      * @param ok if a parsing error occurs ok is set to false, otherwise it's set to true.
      * @param error_msg contains a string explaining the failure reason
      **/
      void parsingFinished(const QVariant& json, bool ok, const QString& error_msg);

    private:
      Q_DISABLE_COPY(ParserRunnable)
      class Private;
      Private* const d;
  };
}

#endif // PARSERRUNNABLE_H
