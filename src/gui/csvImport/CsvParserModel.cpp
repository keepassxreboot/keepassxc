/*
 *  Copyright (C) 2016 Enrico Mariotti <enricomariotti@yahoo.it>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include "CsvParserModel.h"

#include "core/Tools.h"
#include "format/CsvParser.h"

#include <QFile>

CsvParserModel::CsvParserModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_parser(new CsvParser())
    , m_skipped(0)
{
}

CsvParserModel::~CsvParserModel() = default;

CsvParser* CsvParserModel::parser()
{
    return m_parser;
}

void CsvParserModel::setFilename(const QString& filename)
{
    m_filename = filename;
}

QString CsvParserModel::getFileInfo()
{
    return QString("%1, %2, %3")
        .arg(Tools::humanReadableFileSize(m_parser->getFileSize()),
             tr("%n row(s)", "CSV row count", m_parser->getCsvRows()),
             tr("%n column(s)", "CSV column count", qMax(0, m_parser->getCsvCols() - 1)));
}

bool CsvParserModel::parse()
{
    bool r;
    beginResetModel();
    m_columnMap.clear();
    if (m_parser->isFileLoaded()) {
        r = m_parser->reparse();
    } else {
        QFile csv(m_filename);
        r = m_parser->parse(&csv);
    }
    for (int i = 0; i < columnCount(); ++i) {
        m_columnMap.insert(i, 0);
    }
    endResetModel();
    return r;
}

void CsvParserModel::mapColumns(int csvColumn, int dbColumn)
{
    if (dbColumn < 0 || dbColumn >= m_columnMap.size()) {
        return;
    }
    beginResetModel();
    if (csvColumn < 0 || csvColumn >= m_parser->getCsvCols()) {
        // This indicates a blank cell
        m_columnMap[dbColumn] = -1;
    } else {
        m_columnMap[dbColumn] = csvColumn;
    }
    endResetModel();
}

void CsvParserModel::setSkippedRows(int skipped)
{
    m_skipped = skipped;
    QModelIndex topLeft = createIndex(skipped, 0);
    QModelIndex bottomRight = createIndex(m_skipped + rowCount(), columnCount());
    emit dataChanged(topLeft, bottomRight);
    emit layoutChanged();
}

void CsvParserModel::setHeaderLabels(const QStringList& labels)
{
    m_columnHeader = labels;
}

int CsvParserModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_parser->getCsvRows();
}

int CsvParserModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_columnHeader.size();
}

QVariant CsvParserModel::data(const QModelIndex& index, int role) const
{
    if (index.column() >= m_columnHeader.size() || index.row() + m_skipped >= rowCount() || !index.isValid()) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        auto column = m_columnMap[index.column()];
        if (column >= 0) {
            return m_parser->getCsvTable().at(index.row() + m_skipped).at(column);
        }
    }
    return {};
}

QVariant CsvParserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if (section >= 0 && section < m_columnHeader.size()) {
                return m_columnHeader.at(section);
            }
        } else if (orientation == Qt::Vertical) {
            if (section + m_skipped < rowCount()) {
                return QString::number(section + 1);
            }
        }
    }
    return {};
}
