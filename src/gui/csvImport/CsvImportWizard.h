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

#ifndef KEEPASSX_CSVIMPORTWIZARD_H
#define KEEPASSX_CSVIMPORTWIZARD_H

#include "CsvImportWidget.h"

#include <QStackedWidget>
#include <QGridLayout>

#include "core/Database.h"
#include "gui/ChangeMasterKeyWidget.h"
#include "gui/DialogyWidget.h"

class CsvImportWidget;

class CsvImportWizard : public DialogyWidget
{
    Q_OBJECT

public:
    explicit CsvImportWizard(QWidget *parent = nullptr);
    ~CsvImportWizard();
    void load(const QString& filename, Database *database);

signals:
    void importFinished(bool accepted);

private slots:
    void keyFinished(bool accepted);
    void parseFinished(bool accepted);

private:
    Database* m_db;
    CsvImportWidget* parse;
    ChangeMasterKeyWidget* key;
    QStackedWidget *m_pages;
    QGridLayout *m_layout;
};

#endif //KEEPASSX_CSVIMPORTWIZARD_H
