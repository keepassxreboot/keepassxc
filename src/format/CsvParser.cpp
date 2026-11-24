/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2016 Enrico Mariotti <enricomariotti@yahoo.it>
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

#include "CsvParser.h"

#include <QFile>

#include "core/Tools.h"

CsvParser::CsvParser()
    : m_comment('#')
    , m_isBackslashSyntax(false)
    , m_isFileLoaded(false)
    , m_qualifier('"')
    , m_separator(',')
{
    reset();
    m_csv.setBuffer(&m_array);
    m_ts.setDevice(&m_csv);
    m_csv.open(QIODevice::ReadOnly);
}

CsvParser::~CsvParser()
{
    m_csv.close();
}

bool CsvParser::isFileLoaded()
{
    return m_isFileLoaded;
}

bool CsvParser::reparse()
{
    reset();
    return parseFile();
}

bool CsvParser::parse(QIODevice* device)
{
    clear();
    if (!device) {
        appendStatusMsg(QObject::tr("NULL device"), true);
        return false;
    }
    if (!readFile(device)) {
        return false;
    }
    return parseFile();
}

bool CsvParser::readFile(QIODevice* device)
{
    if (device->isOpen()) {
        device->close();
    }

    device->open(QIODevice::ReadOnly);
    if (!Tools::readAllFromDevice(device, m_array)) {
        appendStatusMsg(QObject::tr("error reading from device"), true);
        m_isFileLoaded = false;
    } else {
        device->close();

        // Normalize on newline endings
        m_array.replace("\r\n", "\n");
        m_array.replace("\r", "\n");
        if (m_array.isEmpty()) {
            appendStatusMsg(QObject::tr("file empty").append("\n"));
        }
        m_isFileLoaded = true;
    }
    return m_isFileLoaded;
}

void CsvParser::reset()
{
    m_ch = QChar(0);
    m_currCol = 1;
    m_currRow = 1;
    m_isEof = false;
    m_isGood = true;
    m_lastPos = -1;
    m_maxCols = 0;
    m_statusMsg.clear();
    m_ts.seek(0);
    m_pos = 0;
    m_text.clear();
    m_table.clear();
    // the following can be overridden by the user
    // m_comment = '#';
    // m_backslashSyntax = false;
    // m_comment = '#';
    // m_qualifier = '"';
    // m_separator = ',';
}

void CsvParser::clear()
{
    reset();
    m_isFileLoaded = false;
    m_array.clear();
}

// decode the whole buffer once (honours the codec and BOM handling of QTextStream) so that the parser can
// walk it by index
void CsvParser::loadText()
{
    m_ts.seek(0);
    m_text.clear();
    // QTextStream::readAll() takes seconds for tens of MB; reading in chunks stays linear
    m_text.reserve(m_array.size());
    while (!m_ts.atEnd()) {
        const auto chunk = m_ts.read(1 << 20);
        if (chunk.isEmpty()) {
            break;
        }
        m_text.append(chunk);
    }
    m_pos = 0;
}

bool CsvParser::parseFile()
{
    loadText();
    parseRecord();
    while (!m_isEof) {
        if (!skipEndline()) {
            appendStatusMsg(QObject::tr("malformed string, possible unescaped delimiter"), true);
        }
        m_currRow++;
        m_currCol = 1;
        parseRecord();
    }
    fillColumns();
    // The raw bytes are kept for reparse(); the decoded copy is not needed any more
    m_text = QString();
    return m_isGood;
}

void CsvParser::parseRecord()
{
    CsvRow row;
    if (isComment()) {
        skipLine();
        return;
    }
    do {
        parseField(row);
        getChar(m_ch);
    } while (m_ch == m_separator && !m_isEof);

    if (!m_isEof) {
        ungetChar();
    }
    if (isEmptyRow(row)) {
        row.clear();
        return;
    }
    m_table.push_back(row);
    if (m_maxCols < row.size()) {
        m_maxCols = row.size();
    }
    m_currCol++;
}

void CsvParser::parseField(CsvRow& row)
{
    QString field;
    peek(m_ch);
    if (m_ch != m_separator && m_ch != '\n') {
        if (isQualifier(m_ch)) {
            parseQuoted(field);
        } else {
            parseSimple(field);
        }
    }
    row.push_back(field);
}

void CsvParser::parseSimple(QString& s)
{
    QChar c;
    getChar(c);
    while (c != '\n' && c != m_separator && !m_isEof) {
        s.append(c);
        getChar(c);
    }
    if (!m_isEof) {
        ungetChar();
    }
}

void CsvParser::parseQuoted(QString& s)
{
    // read and discard initial qualifier (e.g. quote)
    getChar(m_ch);
    parseEscaped(s);
    if (!isQualifier(m_ch)) {
        appendStatusMsg(QObject::tr("missing closing delimiter"), true);
    }
}

void CsvParser::parseEscaped(QString& s)
{
    parseEscapedText(s);
    while (processEscapeMark(s, m_ch)) {
        parseEscapedText(s);
    }
    if (!m_isEof) {
        ungetChar();
    }
}

void CsvParser::parseEscapedText(QString& s)
{
    getChar(m_ch);
    while (!isQualifier(m_ch) && !m_isEof) {
        s.append(m_ch);
        getChar(m_ch);
    }
}

bool CsvParser::processEscapeMark(QString& s, QChar c)
{
    QChar c2;
    peek(c2);
    if (m_isBackslashSyntax) {
        // escape-character syntax, e.g. \"
        if (c != '\\') {
            return false;
        }
        // consume (and append) second qualifier
        getChar(c2);
        if (m_isEof) {
            c2 = '\\';
            s.append('\\');
            return false;
        }
        s.append(c2);
        return true;
    }

    // double quote syntax, e.g. ""
    if (!isQualifier(c)) {
        return false;
    }
    peek(c2);
    if (!m_isEof) { // not EOF, can read one char
        if (isQualifier(c2)) {
            s.append(c2);
            getChar(c2);
            return true;
        }
    }
    return false;
}

void CsvParser::fillColumns()
{
    // fill shorter rows with empty placeholder columns
    for (int i = 0; i < m_table.size(); ++i) {
        int gap = m_maxCols - m_table.at(i).size();
        if (gap > 0) {
            CsvRow r = m_table.at(i);
            for (int j = 0; j < gap; ++j) {
                r.append(QString(""));
            }
            m_table.replace(i, r);
        }
    }
}

void CsvParser::skipLine()
{
    // Continue at the line break so that skipEndline() consumes it. Without a line break only the last character
    // is left, as before.
    const auto lineBreak = m_text.indexOf('\n', m_pos);
    m_pos = lineBreak >= 0 ? lineBreak : qMax<qint64>(0, m_text.size() - 1);
}

bool CsvParser::skipEndline()
{
    getChar(m_ch);
    return m_ch == '\n';
}

void CsvParser::getChar(QChar& c)
{
    m_isEof = m_pos >= m_text.size();
    if (!m_isEof) {
        m_lastPos = m_pos;
        c = m_text.at(m_pos++);
    }
}

void CsvParser::ungetChar()
{
    if (m_lastPos < 0) {
        qWarning("CSV Parser: unget lower bound exceeded");
        m_isGood = false;
        return;
    }
    m_pos = m_lastPos;
}

void CsvParser::peek(QChar& c)
{
    getChar(c);
    if (!m_isEof) {
        ungetChar();
    }
}

bool CsvParser::isQualifier(const QChar& c) const
{
    if (m_isBackslashSyntax && c != m_qualifier) {
        return c == '\\';
    }
    return c == m_qualifier;
}

bool CsvParser::isComment()
{
    bool result = false;
    QChar c2;
    const qint64 pos = m_pos;

    do {
        getChar(c2);
    } while ((c2 == ' ' || c2 == '\t') && !m_isEof);

    if (c2 == m_comment) {
        result = true;
    }
    m_pos = pos;
    return result;
}

bool CsvParser::isEmptyRow(const CsvRow& row) const
{
    for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
        if (*it != "\n" && *it != "") {
            return false;
        }
    }
    return true;
}

void CsvParser::setBackslashSyntax(bool set)
{
    m_isBackslashSyntax = set;
}

void CsvParser::setComment(const QChar& c)
{
    m_comment = c.unicode();
}

void CsvParser::setCodec(const QString& s)
{
    auto encoding = QStringConverter::encodingForName(s.toLocal8Bit());
    if (encoding) {
        m_ts.setEncoding(*encoding);
    }
}

QStringConverter::Encoding CsvParser::getCodec() const
{
    return m_ts.encoding();
}

void CsvParser::setFieldSeparator(const QChar& c)
{
    m_separator = c.unicode();
}

void CsvParser::setTextQualifier(const QChar& c)
{
    m_qualifier = c.unicode();
}

int CsvParser::getFileSize() const
{
    return m_csv.size();
}

CsvTable CsvParser::getCsvTable() const
{
    return m_table;
}

QString CsvParser::getStatus() const
{
    return m_statusMsg;
}

int CsvParser::getCsvCols() const
{
    if (!m_table.isEmpty() && !m_table.at(0).isEmpty()) {
        return m_table.at(0).size();
    }
    return 0;
}

int CsvParser::getCsvRows() const
{
    return m_table.size();
}

void CsvParser::appendStatusMsg(const QString& s, bool isCritical)
{
    if (!m_statusMsg.isEmpty()) {
        m_statusMsg.append("\n");
    }

    m_statusMsg +=
        QObject::tr("%1, row: %2, column: %3").arg(s, QString::number(m_currRow), QString::number(m_currCol));

    m_isGood = !isCritical;
}
