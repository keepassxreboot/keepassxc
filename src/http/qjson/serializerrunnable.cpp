#include "serializerrunnable.h"

/* This file is part of qjson
 *
 * Copyright (C) 2009 Flavio Castelli <flavio@castelli.name>
 *               2009 Frank Osterfeld <osterfeld@kde.org>
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

#include "parserrunnable.h"
#include "serializer.h"

#include <QtCore/QDebug>
#include <QtCore/QVariant>

using namespace QJson;

class SerializerRunnable::Private
{
public:
  QVariant json;
};

SerializerRunnable::SerializerRunnable(QObject* parent)
    : QObject(parent),
      QRunnable(),
      d(new Private)
{
  qRegisterMetaType<QVariant>("QVariant");
}

SerializerRunnable::~SerializerRunnable()
{
  delete d;
}

void SerializerRunnable::setJsonObject( const QVariant& json )
{
  d->json = json;
}

void SerializerRunnable::run()
{
  Serializer serializer;
  bool ok;
  const QByteArray serialized = serializer.serialize( d->json, &ok);
  emit parsingFinished( serialized, ok, serializer.errorMessage() );
}
