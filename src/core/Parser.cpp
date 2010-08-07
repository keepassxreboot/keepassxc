/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Parser.h"

#include <QtCore/QFile>

Parser::Parser(Database* db)
{
    m_db = db;
}

bool Parser::parse(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    m_xml.setDevice(&file);


    if (m_xml.readNextStartElement()) {
        if (m_xml.name() == "KeePassFile") {
            parseKeePassFile();
        }
        else {
            m_xml.raiseError(tr("Invalid database file"));
        }
    }

    return !m_xml.error();
}

void Parser::parseKeePassFile()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "KeePassFile");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Meta") {
            parseMeta();
        }
        else if (m_xml.name() == "Root") {
            parseRoot();
        }
        else {
            m_xml.raiseError(tr("Invalid database file"));
        }
    }
}

void Parser::parseMeta()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Meta");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Generator") {

        }
        else if (m_xml.name() == "DatabaseName") {

        }
        else if (m_xml.name() == "DatabaseNameChanged") {

        }
        else if (m_xml.name() == "DatabaseDescription") {

        }
        else if (m_xml.name() == "DatabaseDescriptionChanged") {

        }
        else if (m_xml.name() == "DefaultUserName") {

        }
        else if (m_xml.name() == "DefaultUserNameChanged") {

        }
        else if (m_xml.name() == "MaintenanceHistoryDays") {

        }
        else if (m_xml.name() == "MemoryProtection") {
            parseMemoryProtection();
        }
        else if (m_xml.name() == "CustomIcons") {
            parseCustomIcons();
        }
        else if (m_xml.name() == "RecycleBinEnabled") {

        }
        else if (m_xml.name() == "RecycleBinUUID") {

        }
        else if (m_xml.name() == "RecycleBinChanged") {

        }
        else if (m_xml.name() == "EntryTemplatesGroup") {

        }
        else if (m_xml.name() == "EntryTemplatesGroupChanged") {

        }
        else if (m_xml.name() == "LastSelectedGroup") {

        }
        else if (m_xml.name() == "LastTopVisibleGroup") {

        }
        else if (m_xml.name() == "CustomData") {
            parseCustomData();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseMemoryProtection()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "MemoryProtection");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "ProtectTitle") {

        }
        else if (m_xml.name() == "ProtectUserName") {

        }
        else if (m_xml.name() == "ProtectPassword") {

        }
        else if (m_xml.name() == "ProtectURL") {

        }
        else if (m_xml.name() == "ProtectNotes") {

        }
        else if (m_xml.name() == "AutoEnableVisualHiding") {

        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseCustomIcons()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "CustomIcons");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Icon") {
            parseIcon();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseIcon()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Icon");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {

        }
        else if (m_xml.name() == "Data") {

        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseCustomData()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "CustomData");

    // TODO
}

void Parser::parseRoot()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Root");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Group") {
            parseGroup();
        }
        else if (m_xml.name() == "DeletedObjects") {
            // ?????????????????
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseGroup()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Group");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {

        }
        else if (m_xml.name() == "Name") {

        }
        else if (m_xml.name() == "Notes") {

        }
        else if (m_xml.name() == "IconID") {

        }
        else if (m_xml.name() == "Times") {
            parseTimes();
        }
        else if (m_xml.name() == "IsExpanded") {

        }
        else if (m_xml.name() == "DefaultAutoTypeSequence") {

        }
        else if (m_xml.name() == "EnableAutoType") {

        }
        else if (m_xml.name() == "EnableSearching") {

        }
        else if (m_xml.name() == "LastTopVisibleEntry") {

        }
        else if (m_xml.name() == "Group") {
            parseGroup();
        }
        else if (m_xml.name() == "Entry") {
            parseEntry();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseEntry()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Entry");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "UUID") {

        }
        else if (m_xml.name() == "IconID") {

        }
        else if (m_xml.name() == "CustomIconUUID") {

        }
        else if (m_xml.name() == "ForegroundColor") {

        }
        else if (m_xml.name() == "BackgroundColor") {

        }
        else if (m_xml.name() == "OverrideURL") {

        }
        else if (m_xml.name() == "Times") {
            parseTimes();
        }
        else if (m_xml.name() == "String") {
            parseEntryString();
        }
        else if (m_xml.name() == "Binary") {
            parseEntryBinary();
        }
        else if (m_xml.name() == "AutoType") {
            parseAutoType();
        }
        else if (m_xml.name() == "History") {
            parseEntryHistory();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseEntryString()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "String");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {

        }
        else if (m_xml.name() == "Value") {

        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseEntryBinary()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Binary");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Key") {

        }
        else if (m_xml.name() == "Value") {

        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseAutoType()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "AutoType");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Enabled") {

        }
        else if (m_xml.name() == "DataTransferObfuscation") {

        }
        else if (m_xml.name() == "Association") {
            parseAutoTypeAssoc();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseAutoTypeAssoc()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Association");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Window") {

        }
        else if (m_xml.name() == "KeystrokeSequence") {

        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseEntryHistory()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "History");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "Entry") {
            parseEntry();
        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

void Parser::parseTimes()
{
    Q_ASSERT(m_xml.isStartElement() && m_xml.name() == "Times");

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == "LastModificationTime") {

        }
        else if (m_xml.name() == "CreationTime") {

        }
        else if (m_xml.name() == "LastAccessTime") {

        }
        else if (m_xml.name() == "ExpiryTime") {

        }
        else if (m_xml.name() == "Expires") {

        }
        else if (m_xml.name() == "UsageCount") {

        }
        else if (m_xml.name() == "LocationChanged") {

        }
        else {
            m_xml.skipCurrentElement();
        }
    }
}

QString Parser::readString()
{
    return m_xml.readElementText();
}

bool Parser::readBool()
{
    QString str = readString();

    if (str == "True") {
        return true;
    }
    else if (str == "False") {
        return false;
    }
    else {
        // TODO error
    }
}

QDateTime Parser::readDateTime()
{
    QString str = readString();
    QDateTime dt = QDateTime::fromString(str, Qt::ISODate);

    if (!dt.isValid()) {
        // TODO error
    }

    return dt;
}

int Parser::readInt()
{
    // TODO
}

Uuid Parser::readUuid()
{
    // TODO
}

QByteArray Parser::readBinary()
{
    // TODO
}
