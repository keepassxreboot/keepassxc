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

#include "CsvImportWidget.h"
#include "ui_CsvImportWidget.h"

#include <QFile>
#include <QFileInfo>

#include "gui/MessageBox.h"
#include "gui/MessageWidget.h"

//I wanted to make the CSV import GUI future-proof, so if one day you need entries
//to have a new field, all you have to do is uncomment a row or two here, and the GUI will follow:
//dynamic generation of comboBoxes, labels, placement and so on. Try it for immense fun!
const QStringList CsvImportWidget::m_columnHeader = QStringList()
    << QObject::tr("Group")
    << QObject::tr("Title")
    << QObject::tr("Username")
    << QObject::tr("Password")
    << QObject::tr("URL")
    << QObject::tr("Notes")
//  << QObject::tr("Future field1")
//  << QObject::tr("Future field2")
//  << QObject::tr("Future field3")
    ;

CsvImportWidget::CsvImportWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::CsvImportWidget())
    , m_parserModel(new CsvParserModel(this))
    , m_comboModel(new QStringListModel(this))
    , m_comboMapper(new QSignalMapper(this))
{
    m_ui->setupUi(this);

    m_ui->comboBoxCodec->addItems(QStringList() <<"UTF-8" <<"Windows-1252" <<"UTF-16" <<"UTF-16LE");
    m_ui->comboBoxFieldSeparator->addItems(QStringList() <<"," <<";" <<"-" <<":" <<".");
    m_ui->comboBoxTextQualifier->addItems(QStringList() <<"\"" <<"'" <<":" <<"." <<"|");
    m_ui->comboBoxComment->addItems(QStringList() <<"#" <<";" <<":" <<"@");

    m_ui->tableViewFields->setSelectionMode(QAbstractItemView::NoSelection);
    m_ui->tableViewFields->setFocusPolicy(Qt::NoFocus);

    for (int i = 0; i < m_columnHeader.count(); ++i) {
        QLabel* label = new QLabel(m_columnHeader.at(i), this);
        label->setFixedWidth(label->minimumSizeHint().width());
        QFont font = label->font();
        font.setBold(false);
        label->setFont(font);

        QComboBox* combo = new QComboBox(this);
        font = combo->font();
        font.setBold(false);
        combo->setFont(font);
        m_combos.append(combo);
        combo->setModel(m_comboModel);
        m_comboMapper->setMapping(combo, i);
        connect(combo, SIGNAL(currentIndexChanged(int)), m_comboMapper, SLOT(map()));

        //layout labels and combo fields in column-first order
        int combo_rows = 1 + (m_columnHeader.count() - 1) / 2;
        int x = i % combo_rows;
        int y = 2 * (i / combo_rows);
        m_ui->gridLayout_combos->addWidget(label, x, y);
        m_ui->gridLayout_combos->addWidget(combo, x, y+1);
    }

    m_parserModel->setHeaderLabels(m_columnHeader);
    m_ui->tableViewFields->setModel(m_parserModel);

    connect(m_ui->spinBoxSkip, SIGNAL(valueChanged(int)), SLOT(skippedChanged(int)));
    connect(m_ui->comboBoxCodec, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->comboBoxTextQualifier, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->comboBoxComment, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->comboBoxFieldSeparator, SIGNAL(currentIndexChanged(int)), SLOT(parse()));
    connect(m_ui->checkBoxBackslash, SIGNAL(toggled(bool)), SLOT(parse()));
    connect(m_comboMapper, SIGNAL(mapped(int)), this, SLOT(comboChanged(int)));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(writeDatabase()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CsvImportWidget::comboChanged(int comboId) {
    QComboBox* currentSender = qobject_cast<QComboBox*>(m_comboMapper->mapping(comboId));
    if (currentSender->currentIndex() != -1)
        //this line is the one that actually updates GUI table
        m_parserModel->mapColumns(currentSender->currentIndex(), comboId);
    updateTableview();
}

void CsvImportWidget::skippedChanged(int rows) {
    m_parserModel->setSkippedRows(rows);
    updateTableview();
}

CsvImportWidget::~CsvImportWidget() {}

void CsvImportWidget::configParser() {
    m_parserModel->setBackslashSyntax(m_ui->checkBoxBackslash->isChecked());
    m_parserModel->setComment(m_ui->comboBoxComment->currentText().at(0));
    m_parserModel->setTextQualifier(m_ui->comboBoxTextQualifier->currentText().at(0));
    m_parserModel->setCodec(m_ui->comboBoxCodec->currentText());
    m_parserModel->setFieldSeparator(m_ui->comboBoxFieldSeparator->currentText().at(0));
}

void CsvImportWidget::updateTableview() {
    m_ui->tableViewFields->resizeRowsToContents();
    m_ui->tableViewFields->resizeColumnsToContents();

    for (int c = 0; c < m_ui->tableViewFields->horizontalHeader()->count(); ++c)
        m_ui->tableViewFields->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
}

void CsvImportWidget::updatePreview() {

    m_ui->labelSizeRowsCols->setText(m_parserModel->getFileInfo());
    m_ui->spinBoxSkip->setValue(0);
    m_ui->spinBoxSkip->setMaximum(qMax(0, m_parserModel->rowCount() - 1));

    int i;
    QStringList list(tr("Not present in CSV file"));

    for (i = 1; i < m_parserModel->getCsvCols(); i++) {
        QString s = QString(tr("Column ")) + QString::number(i);
        list << s;
    }
    m_comboModel->setStringList(list);

    i=1;
    Q_FOREACH (QComboBox* b, m_combos) {
        if (i < m_parserModel->getCsvCols())
            b->setCurrentIndex(i);
        else
            b->setCurrentIndex(0);
        ++i;
    }
}

void CsvImportWidget::load(const QString& filename, Database* const db) {
    //QApplication::processEvents();
    m_db = db;
    m_parserModel->setFilename(filename);
    m_ui->labelFilename->setText(filename);
    Group* group = m_db->rootGroup();
    group->setUuid(Uuid::random());
    group->setNotes(tr("Imported from CSV file").append("\n").append(tr("Original data: ")) + filename);
    parse();
}

void CsvImportWidget::parse() {
    configParser();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    //QApplication::processEvents();
    bool good = m_parserModel->parse();
    updatePreview();
    QApplication::restoreOverrideCursor();
    if (good)
        //four newline are provided to avoid resizing at every Positive/Warning switch
        m_ui->messageWidget->showMessage(QString("\n\n").append(tr("CSV syntax seems in good shape !"))
                                         .append("\n\n"), MessageWidget::Positive);
    else
        m_ui->messageWidget->showMessage(tr("Error(s) detected in CSV file !").append("\n")
                                         .append(formatStatusText()), MessageWidget::Warning);
    QWidget::adjustSize();
}


QString CsvImportWidget::formatStatusText() const {
    QString text = m_parserModel->getStatus();
    int items = text.count('\n');
    if (items > 3)
        return text.section('\n', 0, 2)
                .append("\n[").append(QString::number(items - 3))
                .append(tr(" more messages skipped]"));
    else
        for (int i = 0; i < 3 - items; i++)
            text.append(QString("\n"));
        return text;
}

void CsvImportWidget::writeDatabase() {

    setRootGroup();
    for (int r = 0; r < m_parserModel->rowCount(); r++) {
        //use validity of second column as a GO/NOGO for all others fields
        if (not m_parserModel->data(m_parserModel->index(r, 1)).isValid())
            continue;
        Entry* entry = new Entry();
        entry->setUuid(Uuid::random());
        entry->setGroup(splitGroups(m_parserModel->data(m_parserModel->index(r, 0)).toString()));
        entry->setTitle(m_parserModel->data(m_parserModel->index(r, 1)).toString());
        entry->setUsername(m_parserModel->data(m_parserModel->index(r, 2)).toString());
        entry->setPassword(m_parserModel->data(m_parserModel->index(r, 3)).toString());
        entry->setUrl(m_parserModel->data(m_parserModel->index(r, 4)).toString());
        entry->setNotes(m_parserModel->data(m_parserModel->index(r, 5)).toString());
    }
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);

    KeePass2Writer writer;
    writer.writeDatabase(&buffer, m_db);
    if (writer.hasError())
        MessageBox::warning(this, tr("Error"), tr("CSV import: writer has errors:\n")
                   .append((writer.errorString())), QMessageBox::Ok, QMessageBox::Ok);
    emit editFinished(true);
}


void CsvImportWidget::setRootGroup() {
    QString groupLabel;
    QStringList groupList;
    bool is_root  = false;
    bool is_empty = false;
    bool is_label = false;

    for (int r = 0; r < m_parserModel->rowCount(); r++) {
        //use validity of second column as a GO/NOGO for all others fields
        if (not m_parserModel->data(m_parserModel->index(r, 1)).isValid())
            continue;
        groupLabel = m_parserModel->data(m_parserModel->index(r, 0)).toString();
        //check if group name is either "root", "" (empty) or some other label
        groupList = groupLabel.split("/", QString::SkipEmptyParts);
        if (groupList.isEmpty())
            is_empty = true;
        else
            if (not groupList.first().compare("Root", Qt::CaseSensitive))
                is_root = true;
            else if (not groupLabel.compare(""))
                is_empty = true;
            else
                is_label = true;

        groupList.clear();
    }

    if ((is_empty and is_root) or (is_label and not is_empty and is_root))
        m_db->rootGroup()->setName("CSV IMPORTED");
    else
        m_db->rootGroup()->setName("Root");
}

Group *CsvImportWidget::splitGroups(QString label) {
    //extract group names from nested path provided in "label"
    Group *current = m_db->rootGroup();
    if (label.isEmpty())
        return current;

    QStringList groupList = label.split("/", QString::SkipEmptyParts);
    //avoid the creation of a subgroup with the same name as Root
    if (m_db->rootGroup()->name() == "Root" && groupList.first() == "Root")
        groupList.removeFirst();

    for (const QString& groupName : groupList) {
        Group *children = hasChildren(current, groupName);
        if (children == nullptr) {
            Group *brandNew = new Group();
            brandNew->setParent(current);
            brandNew->setName(groupName);
            current = brandNew;
        } else {
            Q_ASSERT(children != nullptr);
            current = children;
        }
    }
    return current;
}

Group* CsvImportWidget::hasChildren(Group* current, QString groupName) {
    //returns the group whose name is "groupName" and is child of "current" group
    for (Group * group : current->children()) {
        if (group->name() == groupName)
            return group;
    }
    return nullptr;
}

void CsvImportWidget::reject() {
    emit editFinished(false);
}
