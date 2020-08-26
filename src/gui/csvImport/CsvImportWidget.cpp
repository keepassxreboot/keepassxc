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

#include "CsvImportWidget.h"
#include "ui_CsvImportWidget.h"

#include <QFile>
#include <QFileInfo>
#include <QSpacerItem>

#include "core/Clock.h"
#include "format/KeePass2Writer.h"
#include "gui/MessageBox.h"
#include "gui/MessageWidget.h"
#include "totp/totp.h"

// I wanted to make the CSV import GUI future-proof, so if one day you need a new field,
// all you have to do is add a field to m_columnHeader, and the GUI will follow:
// dynamic generation of comboBoxes, labels, placement and so on. Try it for immense fun!

CsvImportWidget::CsvImportWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::CsvImportWidget())
    , m_parserModel(new CsvParserModel(this))
    , m_comboModel(new QStringListModel(this))
    , m_columnHeader(QStringList() << QObject::tr("Group") << QObject::tr("Title") << QObject::tr("Username")
                                   << QObject::tr("Password") << QObject::tr("URL") << QObject::tr("Notes")
                                   << QObject::tr("TOTP") << QObject::tr("Icon") << QObject::tr("Last Modified")
                                   << QObject::tr("Created"))
    , m_fieldSeparatorList(QStringList() << ","
                                         << ";"
                                         << "-"
                                         << ":"
                                         << "."
                                         << "\t")
{
    m_ui->setupUi(this);

    m_ui->tableViewFields->setSelectionMode(QAbstractItemView::NoSelection);
    m_ui->tableViewFields->setFocusPolicy(Qt::NoFocus);
    m_ui->messageWidget->setHidden(true);

    m_combos << m_ui->groupCombo << m_ui->titleCombo << m_ui->usernameCombo << m_ui->passwordCombo << m_ui->urlCombo
             << m_ui->notesCombo << m_ui->totpCombo << m_ui->iconCombo << m_ui->lastModifiedCombo << m_ui->createdCombo;

    for (auto combo : m_combos) {
        combo->setModel(m_comboModel);
        connect(combo, SIGNAL(currentIndexChanged(int)), SLOT(comboChanged(int)));
    }

    m_parserModel->setHeaderLabels(m_columnHeader);
    m_ui->tableViewFields->setModel(m_parserModel);

    connect(m_ui->spinBoxSkip, SIGNAL(valueChanged(int)), SLOT(skippedChanged(int)));
    connect(m_ui->comboBoxCodec, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->comboBoxTextQualifier, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->comboBoxComment, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->comboBoxFieldSeparator, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->checkBoxBackslash, SIGNAL(toggled(bool)), SLOT(parse()));
    connect(m_ui->checkBoxFieldNames, SIGNAL(toggled(bool)), SLOT(updatePreview()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(writeDatabase()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CsvImportWidget::comboChanged(int index)
{
    // this line is the one that actually updates GUI table
    m_parserModel->mapColumns(index, m_combos.indexOf(qobject_cast<QComboBox*>(sender())));
    updateTableview();
}

void CsvImportWidget::skippedChanged(int rows)
{
    m_parserModel->setSkippedRows(rows);
    updateTableview();
}

CsvImportWidget::~CsvImportWidget()
{
}

void CsvImportWidget::configParser()
{
    m_parserModel->setBackslashSyntax(m_ui->checkBoxBackslash->isChecked());
    m_parserModel->setComment(m_ui->comboBoxComment->currentText().at(0));
    m_parserModel->setTextQualifier(m_ui->comboBoxTextQualifier->currentText().at(0));
    m_parserModel->setCodec(m_ui->comboBoxCodec->currentText());
    m_parserModel->setFieldSeparator(m_fieldSeparatorList.at(m_ui->comboBoxFieldSeparator->currentIndex()).at(0));
}

void CsvImportWidget::updateTableview()
{
    m_ui->tableViewFields->resizeRowsToContents();
    m_ui->tableViewFields->resizeColumnsToContents();

    for (int c = 0; c < m_ui->tableViewFields->horizontalHeader()->count(); ++c) {
        m_ui->tableViewFields->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
    }
}

void CsvImportWidget::updatePreview()
{
    int minSkip = 0;
    if (m_ui->checkBoxFieldNames->isChecked()) {
        minSkip = 1;
    }
    m_ui->labelSizeRowsCols->setText(m_parserModel->getFileInfo());
    m_ui->spinBoxSkip->setRange(minSkip, qMax(minSkip, m_parserModel->rowCount() - 1));
    m_ui->spinBoxSkip->setValue(minSkip);

    QStringList list(tr("Not Present"));
    for (int i = 1; i < m_parserModel->getCsvCols(); ++i) {
        if (m_ui->checkBoxFieldNames->isChecked()) {
            auto columnName = m_parserModel->getCsvTable().at(0).at(i);
            if (columnName.isEmpty()) {
                list << QString(tr("Column %1").arg(i));
            } else {
                list << columnName;
            }
        } else {
            list << QString(tr("Column %1").arg(i));
        }
    }
    m_comboModel->setStringList(list);

    int j = 1;
    for (QComboBox* b : m_combos) {
        if (j < m_parserModel->getCsvCols()) {
            b->setCurrentIndex(j);
        } else {
            b->setCurrentIndex(0);
        }
        ++j;
    }
}

void CsvImportWidget::load(const QString& filename, Database* const db)
{
    // QApplication::processEvents();
    m_db = db;
    m_parserModel->setFilename(filename);
    m_ui->labelFilename->setText(filename);
    Group* group = m_db->rootGroup();
    group->setUuid(QUuid::createUuid());
    group->setNotes(tr("Imported from CSV file").append("\n").append(tr("Original data: ")) + filename);
    parse();
}

void CsvImportWidget::parse()
{
    configParser();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    // QApplication::processEvents();
    bool good = m_parserModel->parse();
    updatePreview();
    QApplication::restoreOverrideCursor();
    if (!good) {
        m_ui->messageWidget->showMessage(tr("Error(s) detected in CSV file!").append("\n").append(formatStatusText()),
                                         MessageWidget::Warning);
    } else {
        m_ui->messageWidget->setHidden(true);
    }
}

QString CsvImportWidget::formatStatusText() const
{
    QString text = m_parserModel->getStatus();
    int items = text.count('\n');
    if (items > 2) {
        return text.section('\n', 0, 1).append("\n").append(tr("[%n more message(s) skipped]", "", items - 2));
    }
    if (items == 1) {
        text.append(QString("\n"));
    }
    return text;
}

void CsvImportWidget::writeDatabase()
{
    setRootGroup();
    for (int r = 0; r < m_parserModel->rowCount(); ++r) {
        // use validity of second column as a GO/NOGO for all others fields
        if (not m_parserModel->data(m_parserModel->index(r, 1)).isValid()) {
            continue;
        }
        Entry* entry = new Entry();
        entry->setUuid(QUuid::createUuid());
        entry->setGroup(splitGroups(m_parserModel->data(m_parserModel->index(r, 0)).toString()));
        entry->setTitle(m_parserModel->data(m_parserModel->index(r, 1)).toString());
        entry->setUsername(m_parserModel->data(m_parserModel->index(r, 2)).toString());
        entry->setPassword(m_parserModel->data(m_parserModel->index(r, 3)).toString());
        entry->setUrl(m_parserModel->data(m_parserModel->index(r, 4)).toString());
        entry->setNotes(m_parserModel->data(m_parserModel->index(r, 5)).toString());

        if (m_parserModel->data(m_parserModel->index(r, 6)).isValid()) {
            auto totp = Totp::parseSettings(m_parserModel->data(m_parserModel->index(r, 6)).toString());
            entry->setTotp(totp);
        }

        bool ok;
        int icon = m_parserModel->data(m_parserModel->index(r, 7)).toInt(&ok);
        if (ok) {
            entry->setIcon(icon);
        }

        TimeInfo timeInfo;
        if (m_parserModel->data(m_parserModel->index(r, 8)).isValid()) {
            auto datetime = m_parserModel->data(m_parserModel->index(r, 8)).toString();
            if (datetime.contains(QRegularExpression("^\\d+$"))) {
                timeInfo.setLastModificationTime(Clock::datetimeUtc(datetime.toLongLong() * 1000));
            } else {
                auto lastModified = QDateTime::fromString(datetime, Qt::ISODate);
                if (lastModified.isValid()) {
                    timeInfo.setLastModificationTime(lastModified);
                }
            }
        }
        if (m_parserModel->data(m_parserModel->index(r, 9)).isValid()) {
            auto datetime = m_parserModel->data(m_parserModel->index(r, 9)).toString();
            if (datetime.contains(QRegularExpression("^\\d+$"))) {
                timeInfo.setCreationTime(Clock::datetimeUtc(datetime.toLongLong() * 1000));
            } else {
                auto created = QDateTime::fromString(datetime, Qt::ISODate);
                if (created.isValid()) {
                    timeInfo.setCreationTime(created);
                }
            }
        }
        entry->setTimeInfo(timeInfo);
    }
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);

    KeePass2Writer writer;
    writer.writeDatabase(&buffer, m_db);
    if (writer.hasError()) {
        MessageBox::warning(this,
                            tr("Error"),
                            tr("CSV import: writer has errors:\n%1").arg(writer.errorString()),
                            MessageBox::Ok,
                            MessageBox::Ok);
    }
    emit editFinished(true);
}

void CsvImportWidget::setRootGroup()
{
    QString groupLabel;
    QStringList groupList;
    bool is_root = false;
    bool is_empty = false;
    bool is_label = false;

    for (int r = 0; r < m_parserModel->rowCount(); ++r) {
        // use validity of second column as a GO/NOGO for all others fields
        if (not m_parserModel->data(m_parserModel->index(r, 1)).isValid()) {
            continue;
        }
        groupLabel = m_parserModel->data(m_parserModel->index(r, 0)).toString();
        // check if group name is either "root", "" (empty) or some other label
        groupList = groupLabel.split("/", QString::SkipEmptyParts);
        if (groupList.isEmpty()) {
            is_empty = true;
        } else if (not groupList.first().compare("Root", Qt::CaseSensitive)) {
            is_root = true;
        } else if (not groupLabel.compare("")) {
            is_empty = true;
        } else {
            is_label = true;
        }

        groupList.clear();
    }

    if ((is_empty and is_root) or (is_label and not is_empty and is_root)) {
        m_db->rootGroup()->setName("CSV IMPORTED");
    } else {
        m_db->rootGroup()->setName("Root");
    }
}

Group* CsvImportWidget::splitGroups(const QString& label)
{
    // extract group names from nested path provided in "label"
    Group* current = m_db->rootGroup();
    if (label.isEmpty()) {
        return current;
    }

    QStringList groupList = label.split("/", QString::SkipEmptyParts);
    // avoid the creation of a subgroup with the same name as Root
    if (m_db->rootGroup()->name() == "Root" && groupList.first() == "Root") {
        groupList.removeFirst();
    }

    for (const QString& groupName : groupList) {
        Group* children = hasChildren(current, groupName);
        if (children == nullptr) {
            Group* brandNew = new Group();
            brandNew->setParent(current);
            brandNew->setName(groupName);
            brandNew->setUuid(QUuid::createUuid());
            current = brandNew;
        } else {
            Q_ASSERT(children != nullptr);
            current = children;
        }
    }
    return current;
}

Group* CsvImportWidget::hasChildren(Group* current, const QString& groupName)
{
    // returns the group whose name is "groupName" and is child of "current" group
    for (Group* group : current->children()) {
        if (group->name() == groupName) {
            return group;
        }
    }
    return nullptr;
}

void CsvImportWidget::reject()
{
    emit editFinished(false);
}
