/* This file is part of qjson
  *
  * Copyright (C) 2009 Till Adam <adam@kde.org>
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

#ifndef PERSON_H
#define PERSON_H

#include <QtCore/QDate>
#include <QtCore/QtGlobal>
#include <QtCore/QObject>
#include <QtCore/QVariant>

class Person : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString name READ name WRITE setName)
  Q_PROPERTY(int phoneNumber READ phoneNumber WRITE setPhoneNumber)
  Q_PROPERTY(Gender gender READ gender WRITE setGender)
  Q_PROPERTY(QDate dob READ dob WRITE setDob)
  Q_PROPERTY(QVariant customField READ customField WRITE setCustomField)
  Q_PROPERTY(quint16 luckyNumber READ luckyNumber WRITE setLuckyNumber)
  Q_ENUMS(Gender)

 public:
    Person(QObject* parent = 0);
    ~Person();

    QString name() const;
    void setName(const QString& name);

    int phoneNumber() const;
    void setPhoneNumber(const int  phoneNumber);

    enum Gender {Male, Female};
    void setGender(Gender gender);
    Gender gender() const;

    QDate dob() const;
    void setDob(const QDate& dob);

    QVariant customField() const;
    void setCustomField(const QVariant& customField);

    const quint16 luckyNumber() const;
    void setLuckyNumber(const quint16 luckyNumber);

  private:
    QString m_name;
    int m_phoneNumber;
    Gender m_gender;
    QDate m_dob;
    QVariant m_customField;
    quint16 m_luckyNumber;
};

#endif
