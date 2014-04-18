#include "person.h"

Person::Person(QObject* parent)
  : QObject(parent),
    m_name(),
    m_phoneNumber(0),
    m_gender(Female),
    m_luckyNumber(0)
{
}

Person::~Person()
{
}

QString Person::name() const
{
  return m_name;
}

void Person::setName(const QString& name)
{
  m_name = name;
}

int Person::phoneNumber() const
{
  return m_phoneNumber;
}

void Person::setPhoneNumber(const int phoneNumber)
{
  m_phoneNumber = phoneNumber;
}

void Person::setGender(Gender gender)
{
  m_gender = gender;
}

Person::Gender Person::gender() const
{
  return m_gender;
}

QDate Person::dob() const
{
  return m_dob;
}

void Person::setDob(const QDate& dob)
{
  m_dob = dob;
}

QVariant Person::customField() const
{
  return m_customField;
}

void Person::setCustomField(const QVariant& customField)
{
  m_customField = customField;
}

const quint16 Person::luckyNumber() const
{
  return m_luckyNumber;
}

void Person::setLuckyNumber(const quint16 luckyNumber)
{
  m_luckyNumber = luckyNumber;
}

