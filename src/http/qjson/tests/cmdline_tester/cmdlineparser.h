/* This file is part of qjson
  *
  * Copyright (C) 2010 Flavio Castelli <flavio@castelli.name>
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

#ifndef CMDLINEPARSER_H
#define CMDLINEPARSER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

#include <QJson/Serializer>

namespace QJson {
  class CmdLineParser
  {
    public:
      enum Result {Ok, Help, Error};

      CmdLineParser(const QStringList &arguments);
      Result parse();

      void setIndentationMode(const IndentMode &mode);
      IndentMode indentationMode() const;
      QString helpFile() const;
      QString file() const;
      bool serialize();
      bool quiet();

      void showMessage(const QString &msg, bool error);

    private:
      bool hasMoreArgs() const;
      const QString &nextArg();
      void handleSetIndentationMode();

      QStringList m_arguments;
      int m_pos;
      IndentMode m_indentationMode;
      QString m_file;
      bool m_serialize;
      bool m_quiet;
      static const QString m_helpMessage;
      QString m_error;
  };
}

#endif

