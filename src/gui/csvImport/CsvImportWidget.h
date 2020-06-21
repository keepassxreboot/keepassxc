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

#ifndef KEEPASSX_CSVIMPORTWIDGET_H
#define KEEPASSX_CSVIMPORTWIDGET_H

#include <QComboBox>
#include <QList>
#include <QPushButton>
#include <QScopedPointer>
#include <QStackedWidget>
#include <QStringListModel>

#include "core/Metadata.h"
#include "gui/csvImport/CsvParserModel.h"
#include "keys/PasswordKey.h"

namespace Ui
{
    class CsvImportWidget;
}

class CsvImportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CsvImportWidget(QWidget* parent = nullptr);
    ~CsvImportWidget();
    void load(const QString& filename, Database* const db);

signals:
    void editFinished(bool accepted);

private slots:
    void parse();
    void comboChanged(int index);
    void skippedChanged(int rows);
    void writeDatabase();
    void updatePreview();
    void setRootGroup();
    void reject();

private:
    Q_DISABLE_COPY(CsvImportWidget)
    const QScopedPointer<Ui::CsvImportWidget> m_ui;
    CsvParserModel* const m_parserModel;
    QStringListModel* const m_comboModel;
    QList<QComboBox*> m_combos;
    Database* m_db;

    const QStringList m_columnHeader;
    QStringList m_fieldSeparatorList;
    void configParser();
    void updateTableview();
    Group* splitGroups(const QString& label);
    Group* hasChildren(Group* current, const QString& groupName);
    QString formatStatusText() const;
};

#endif // KEEPASSX_CSVIMPORTWIDGET_H
