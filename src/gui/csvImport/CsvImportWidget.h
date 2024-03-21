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

#include <QStringListModel>
#include <QWidget>

class CsvParserModel;
class Database;
class Group;
class QComboBox;

namespace Ui
{
    class CsvImportWidget;
}

class CsvImportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CsvImportWidget(QWidget* parent = nullptr);
    ~CsvImportWidget() override;

    void load(const QString& filename);
    QSharedPointer<Database> buildDatabase();

signals:
    void message(QString msg);

private slots:
    void parse();
    void comboChanged(int index);
    void skippedChanged(int rows);
    void updatePreview();

private:
    void configParser();
    void updateTableview();
    QString formatStatusText() const;

    QScopedPointer<Ui::CsvImportWidget> m_ui;

    CsvParserModel* m_parserModel;
    QStringListModel* m_comboModel;
    QList<QComboBox*> m_combos;
    QStringList m_columnHeader;
    QStringList m_fieldSeparatorList;
    QString m_filename;
    bool m_buildingPreview = false;

    Q_DISABLE_COPY(CsvImportWidget)
};

#endif // KEEPASSX_CSVIMPORTWIDGET_H
