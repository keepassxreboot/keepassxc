/*
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

#ifndef KEEPASSX_CSVIMPORTWIDGET_H
#define KEEPASSX_CSVIMPORTWIDGET_H

#include <QScopedPointer>
#include <QPushButton>
#include <QStringListModel>
#include <QSignalMapper>
#include <QList>
#include <QComboBox>
#include <QStackedWidget>

#include "format/KeePass2Writer.h"
#include "gui/csvImport/CsvParserModel.h"
#include "keys/PasswordKey.h"
#include "core/Metadata.h"


namespace Ui {
    class CsvImportWidget;
}

class CsvImportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CsvImportWidget(QWidget *parent = nullptr);
    virtual ~CsvImportWidget();
    void load(const QString& filename, Database* const db);

Q_SIGNALS:
    void editFinished(bool accepted);

private Q_SLOTS:
    void parse();
    void showReport();
    void comboChanged(int comboId);
    void skippedChanged(int rows);
    void writeDatabase();
    void reject();

private:
    Q_DISABLE_COPY(CsvImportWidget)
    const QScopedPointer<Ui::CsvImportWidget> m_ui;
    CsvParserModel* const m_parserModel;
    QStringListModel* const m_comboModel;
    QSignalMapper* m_comboMapper;
    QList<QComboBox*> m_combos;
    Database *m_db;
    int m_lastParseColumns;

    KeePass2Writer m_writer;
    static const QStringList m_columnheader;
    void configParser();
    void updatePreview();
    void updateTableview();
    Group* grp(QString label);
    Group* hasChildren(Group* current, QString grpName);
};

#endif // KEEPASSX_CSVIMPORTWIDGET_H
