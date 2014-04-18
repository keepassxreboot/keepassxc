/* This file is part of QJson
 *
 * Copyright (C) 2009 Flavio Castelli <flavio.castelli@gmail.com>
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

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QTextCodec>
#include <QtCore/QTime>

#include <QJson/Parser>
#include <QJson/Serializer>

#include "cmdlineparser.h"

using namespace QJson;

int main(int argc, char *argv[]) {
  QCoreApplication app (argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
  QTextCodec *codec = QTextCodec::codecForName("UTF-8");
  QTextCodec::setCodecForCStrings(codec);
#endif

  QTime time;
  int   duration;

 
  CmdLineParser cmd (app.arguments());
  CmdLineParser::Result res = cmd.parse();
  if (res == CmdLineParser::Help)
      return 0;
  else if (res == CmdLineParser::Error)
      return -1;

  QString filename = cmd.file();
  if (!QFile::exists ( filename )) {
    qCritical ("The file you specified doesn't exist!");
    exit (1);
  }

  Parser parser;
  bool ok;

  QFile file (filename);
  time.start();
  QVariant data = parser.parse (&file, &ok);
  duration = time.elapsed();
  if (!ok) {
    qCritical("%s:%i - Error: %s", filename.toLatin1().data(), parser.errorLine(), qPrintable(parser.errorString()));
    exit (1);
  }
  else {
    qDebug() << "Parsing of" << filename << "took" << duration << "ms";
    if (!cmd.quiet())
      qDebug() << data;
  }

  if (cmd.serialize()) {
    // serializer tests
    qDebug() << "Serializing... ";
    QJson::Serializer serializer;
    serializer.setIndentMode(cmd.indentationMode());
    time.start();
    QByteArray b = serializer.serialize(data, &ok);
    if (!ok) {
      qCritical() << "Serialization failed:" << serializer.errorMessage();
      exit(1);
    } else {
      duration = time.elapsed();
      qDebug() << "Serialization took:" << duration << "ms";
      if (!cmd.quiet())
       qDebug() << b;
    }
  }

  qDebug() << "JOB DONE, BYE";
  return 0;
}

