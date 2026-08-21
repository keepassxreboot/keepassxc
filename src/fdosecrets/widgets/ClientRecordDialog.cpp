/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "ClientRecordDialog.h"
#include "ui_ClientRecordDialog.h"

#include "fdosecrets/widgets/ClientAuthModels.h"

#include "core/Clock.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"

#include <QComboBox>
#include <QCryptographicHash>
#include <QFile>
#include <QFileDialog>
#include <QLocale>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStyledItemDelegate>

namespace FdoSecrets
{
    /**
     * The record's rules as a two level tree: rules at the top, their
     * conditions as editable children. Condition indexes carry their rule row
     * in the internalId (offset by one; zero marks a rule row), so removing a
     * whole rule resets the model instead of fixing up stale ids.
     */
    class MatchRulesModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        enum Column
        {
            ColumnProcess,
            ColumnKind,
            ColumnValue,
        };

        using QAbstractItemModel::QAbstractItemModel;

        void setRules(QList<MatchRule> rules)
        {
            beginResetModel();
            m_rules = std::move(rules);
            endResetModel();
        }

        const QList<MatchRule>& rules() const
        {
            return m_rules;
        }

        bool isCondition(const QModelIndex& index) const
        {
            return index.isValid() && index.internalId() != 0;
        }

        /// The row of the rule the index belongs to (the index itself for rule rows).
        int ruleRow(const QModelIndex& index) const
        {
            return isCondition(index) ? static_cast<int>(index.internalId()) - 1 : index.row();
        }

        QModelIndex addRule()
        {
            const auto row = m_rules.size();
            beginInsertRows({}, row, row);
            // start the rule out with one condition to fill in
            m_rules.append(MatchRule{{RuleCondition{0, RuleCondition::Kind::Path, {}, {}}}});
            endInsertRows();
            return index(row, 0, {});
        }

        QModelIndex addCondition(int rule)
        {
            if (rule < 0 || rule >= m_rules.size()) {
                return {};
            }
            const auto parent = index(rule, 0, {});
            const auto row = m_rules.at(rule).conditions.size();
            beginInsertRows(parent, row, row);
            m_rules[rule].conditions.append(RuleCondition{0, RuleCondition::Kind::Path, {}, {}});
            endInsertRows();
            return index(row, 0, parent);
        }

        void removeIndex(const QModelIndex& index)
        {
            if (!index.isValid()) {
                return;
            }
            if (isCondition(index)) {
                beginRemoveRows(index.parent(), index.row(), index.row());
                m_rules[ruleRow(index)].conditions.removeAt(index.row());
                endRemoveRows();
            } else {
                // condition indexes of later rules would keep their old rule row
                // in the internalId; a reset avoids handing out stale indexes
                beginResetModel();
                m_rules.removeAt(index.row());
                endResetModel();
            }
        }

        QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override
        {
            if (!hasIndex(row, column, parent)) {
                return {};
            }
            if (!parent.isValid()) {
                return createIndex(row, column, quintptr(0));
            }
            return createIndex(row, column, quintptr(parent.row()) + 1);
        }

        QModelIndex parent(const QModelIndex& index) const override
        {
            if (!isCondition(index)) {
                return {};
            }
            return createIndex(ruleRow(index), 0, quintptr(0));
        }

        int rowCount(const QModelIndex& parent = {}) const override
        {
            if (!parent.isValid()) {
                return m_rules.size();
            }
            if (isCondition(parent) || parent.column() != 0) {
                return 0;
            }
            return m_rules.at(parent.row()).conditions.size();
        }

        int columnCount(const QModelIndex& parent = {}) const override
        {
            Q_UNUSED(parent);
            return 3;
        }

        Qt::ItemFlags flags(const QModelIndex& index) const override
        {
            if (!index.isValid()) {
                return Qt::NoItemFlags;
            }
            auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if (isCondition(index)) {
                flags |= Qt::ItemIsEditable;
            }
            return flags;
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override
        {
            if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
                return {};
            }
            switch (section) {
            case ColumnProcess:
                return tr("Process");
            case ColumnKind:
                return tr("Match on");
            case ColumnValue:
                return tr("Value");
            default:
                return {};
            }
        }

        QVariant data(const QModelIndex& index, int role) const override
        {
            if (!index.isValid()) {
                return {};
            }
            if (!isCondition(index)) {
                if (index.column() == ColumnProcess && role == Qt::DisplayRole) {
                    return tr("Rule %1").arg(index.row() + 1);
                }
                return {};
            }
            const auto& cond = m_rules.at(ruleRow(index)).conditions.at(index.row());
            switch (index.column()) {
            case ColumnProcess:
                if (role == Qt::DisplayRole) {
                    return ClientRecordsModel::processLabel(cond.depth);
                }
                if (role == Qt::EditRole) {
                    return cond.depth;
                }
                break;
            case ColumnKind:
                if (role == Qt::DisplayRole) {
                    return ClientRecordsModel::conditionKindLabel(cond.kind, cond.algo);
                }
                if (role == Qt::EditRole) {
                    return static_cast<int>(cond.kind);
                }
                break;
            case ColumnValue:
                if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
                    return cond.value;
                }
                break;
            default:
                break;
            }
            return {};
        }

        bool setData(const QModelIndex& index, const QVariant& value, int role) override
        {
            if (role != Qt::EditRole || !isCondition(index)) {
                return false;
            }
            auto& cond = m_rules[ruleRow(index)].conditions[index.row()];
            switch (index.column()) {
            case ColumnProcess:
                cond.depth = qBound(0, value.toInt(), 99);
                break;
            case ColumnKind: {
                const auto kind = static_cast<RuleCondition::Kind>(value.toInt());
                if (kind != cond.kind) {
                    // a path or name is never a digest and vice versa
                    if (kind == RuleCondition::Kind::Hash || cond.kind == RuleCondition::Kind::Hash) {
                        cond.value.clear();
                    }
                    cond.algo = kind == RuleCondition::Kind::Hash ? DefaultExeHashAlgo : QString();
                    cond.kind = kind;
                }
                break;
            }
            case ColumnValue:
                cond.value =
                    cond.kind == RuleCondition::Kind::Hash ? value.toString().trimmed().toLower() : value.toString();
                break;
            default:
                return false;
            }
            emit dataChanged(index.siblingAtColumn(ColumnProcess), index.siblingAtColumn(ColumnValue));
            return true;
        }

    private:
        QList<MatchRule> m_rules;
    };

    /**
     * The entries of the database carrying a decision for this record. The
     * dialog only supports removing them; removals are staged and read back by
     * the caller on accept.
     */
    class RecordEntryDecisionsModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum Column
        {
            ColumnEntry,
            ColumnAccess,
        };

        using QAbstractTableModel::QAbstractTableModel;

        void load(const Database* db, const DBusClientId& id)
        {
            beginResetModel();
            m_rows.clear();
            m_removed.clear();
            if (db && !id.isNull()) {
                for (auto entry : db->rootGroup()->entriesRecursive()) {
                    const auto decisions = entryClientDecisions(entry);
                    const auto it = decisions.constFind(id);
                    if (it != decisions.constEnd()) {
                        m_rows.append({entry, *it});
                    }
                }
            }
            endResetModel();
        }

        void removeRow(int row)
        {
            if (row < 0 || row >= m_rows.size()) {
                return;
            }
            beginRemoveRows({}, row, row);
            if (m_rows.at(row).first) {
                m_removed.append(m_rows.at(row).first);
            }
            m_rows.removeAt(row);
            endRemoveRows();
        }

        QList<Entry*> removedEntries() const
        {
            QList<Entry*> entries;
            for (const auto& entry : m_removed) {
                if (entry) {
                    entries.append(entry);
                }
            }
            return entries;
        }

        int rowCount(const QModelIndex& parent = {}) const override
        {
            return parent.isValid() ? 0 : m_rows.size();
        }

        int columnCount(const QModelIndex& parent = {}) const override
        {
            Q_UNUSED(parent);
            return 2;
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override
        {
            if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
                return {};
            }
            switch (section) {
            case ColumnEntry:
                return tr("Entry");
            case ColumnAccess:
                return tr("Access");
            default:
                return {};
            }
        }

        QVariant data(const QModelIndex& index, int role) const override
        {
            if (!index.isValid() || index.row() >= m_rows.size()) {
                return {};
            }
            const auto& row = m_rows.at(index.row());
            if (!row.first) {
                return {};
            }
            switch (index.column()) {
            case ColumnEntry:
                if (role == Qt::DisplayRole) {
                    return row.first->title();
                }
                if (role == Qt::ToolTipRole && row.first->group()) {
                    return row.first->group()->hierarchy().join(QLatin1Char('/'));
                }
                break;
            case ColumnAccess:
                if (role == Qt::DisplayRole) {
                    return row.second == AuthDecision::Allowed ? tr("Allow") : tr("Deny");
                }
                break;
            default:
                break;
            }
            return {};
        }

    private:
        QList<QPair<QPointer<Entry>, AuthDecision>> m_rows;
        QList<QPointer<Entry>> m_removed;
    };

    namespace
    {
        class ConditionDelegate : public QStyledItemDelegate
        {
            Q_OBJECT

        public:
            using QStyledItemDelegate::QStyledItemDelegate;

            QWidget*
            createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
            {
                switch (index.column()) {
                case MatchRulesModel::ColumnProcess: {
                    auto editor = new QSpinBox(parent);
                    editor->setRange(0, 99);
                    editor->setToolTip(tr("0 = the calling process, 1 = its parent, and so on"));
                    return editor;
                }
                case MatchRulesModel::ColumnKind: {
                    auto editor = new QComboBox(parent);
                    for (auto kind :
                         {RuleCondition::Kind::Path, RuleCondition::Kind::Name, RuleCondition::Kind::Hash}) {
                        editor->addItem(ClientRecordsModel::conditionKindLabel(kind, DefaultExeHashAlgo),
                                        static_cast<int>(kind));
                    }
                    return editor;
                }
                default:
                    return QStyledItemDelegate::createEditor(parent, option, index);
                }
            }

            void setEditorData(QWidget* editor, const QModelIndex& index) const override
            {
                switch (index.column()) {
                case MatchRulesModel::ColumnProcess:
                    static_cast<QSpinBox*>(editor)->setValue(index.data(Qt::EditRole).toInt());
                    break;
                case MatchRulesModel::ColumnKind: {
                    auto combo = static_cast<QComboBox*>(editor);
                    combo->setCurrentIndex(combo->findData(index.data(Qt::EditRole).toInt()));
                    break;
                }
                default:
                    QStyledItemDelegate::setEditorData(editor, index);
                    break;
                }
            }

            void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
            {
                switch (index.column()) {
                case MatchRulesModel::ColumnProcess:
                    model->setData(index, static_cast<QSpinBox*>(editor)->value(), Qt::EditRole);
                    break;
                case MatchRulesModel::ColumnKind:
                    model->setData(index, static_cast<QComboBox*>(editor)->currentData(), Qt::EditRole);
                    break;
                default:
                    QStyledItemDelegate::setModelData(editor, model, index);
                    break;
                }
            }
        };

        // combo indexes of allEntriesCombo
        constexpr int AllEntriesAsk = 0;
        constexpr int AllEntriesAllow = 1;
        constexpr int AllEntriesDeny = 2;
    } // namespace

    ClientRecordDialog::ClientRecordDialog(QSharedPointer<Database> db,
                                           ClientRecord record,
                                           QList<ClientRecord> siblings,
                                           QWidget* parent)
        : QDialog(parent)
        , m_ui(new Ui::ClientRecordDialog)
        , m_db(std::move(db))
        , m_record(std::move(record))
        , m_rulesModel(new MatchRulesModel(this))
        , m_decisionsModel(new RecordEntryDecisionsModel(this))
    {
        m_ui->setupUi(this);
        m_ui->overlapWarning->setHidden(true);
        m_ui->overlapWarning->setCloseButtonVisible(false);
        m_ui->overlapWarning->setWordWrap(true);

        for (const auto& other : asConst(siblings)) {
            if (other.id != m_record.id) {
                m_otherRecords.append(other);
            }
        }

        m_ui->nameEdit->setText(m_record.name);
        m_ui->createdValue->setText(m_record.id.isNull()
                                        ? tr("on save")
                                        : QLocale().toString(m_record.created.toLocalTime(), QLocale::ShortFormat));

        m_rulesModel->setRules(m_record.rules);
        m_ui->conditionsView->setModel(m_rulesModel);
        m_ui->conditionsView->setItemDelegate(new ConditionDelegate(m_ui->conditionsView));
        m_ui->conditionsView->expandAll();
        m_ui->conditionsView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_ui->conditionsView->header()->setSectionResizeMode(MatchRulesModel::ColumnValue, QHeaderView::Stretch);

        m_decisionsModel->load(m_db.data(), m_record.id);
        m_ui->entryDecisionsView->setModel(m_decisionsModel);
        m_ui->entryDecisionsView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_ui->entryDecisionsView->horizontalHeader()->setSectionResizeMode(RecordEntryDecisionsModel::ColumnEntry,
                                                                           QHeaderView::Stretch);

        switch (m_record.allEntries) {
        case AuthDecision::Allowed:
            m_ui->allEntriesCombo->setCurrentIndex(AllEntriesAllow);
            break;
        case AuthDecision::Denied:
            m_ui->allEntriesCombo->setCurrentIndex(AllEntriesDeny);
            break;
        default:
            m_ui->allEntriesCombo->setCurrentIndex(AllEntriesAsk);
            break;
        }

        connect(m_ui->addRuleButton, &QPushButton::clicked, this, [this]() {
            const auto idx = m_rulesModel->addRule();
            m_ui->conditionsView->expand(idx);
            m_ui->conditionsView->setCurrentIndex(m_rulesModel->index(0, 0, idx));
        });
        connect(m_ui->addConditionButton, &QPushButton::clicked, this, [this]() {
            const auto idx = m_rulesModel->addCondition(m_rulesModel->ruleRow(m_ui->conditionsView->currentIndex()));
            if (idx.isValid()) {
                m_ui->conditionsView->setCurrentIndex(idx);
            }
        });
        connect(m_ui->removeConditionButton, &QPushButton::clicked, this, [this]() {
            m_rulesModel->removeIndex(m_ui->conditionsView->currentIndex());
        });
        connect(m_ui->hashFromFileButton, &QPushButton::clicked, this, &ClientRecordDialog::hashFromFile);
        connect(m_ui->removeDecisionButton, &QPushButton::clicked, this, [this]() {
            m_decisionsModel->removeRow(m_ui->entryDecisionsView->currentIndex().row());
        });

        // a model reset (whole-rule removal) drops the expansion state
        connect(m_rulesModel, &QAbstractItemModel::modelReset, m_ui->conditionsView, &QTreeView::expandAll);

        connect(m_ui->conditionsView->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                &ClientRecordDialog::updateButtons);
        connect(m_ui->entryDecisionsView->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                &ClientRecordDialog::updateButtons);
        // any edit can change what the rules match
        connect(m_rulesModel, &QAbstractItemModel::dataChanged, this, &ClientRecordDialog::updateOverlapWarning);
        connect(m_rulesModel, &QAbstractItemModel::rowsInserted, this, &ClientRecordDialog::updateOverlapWarning);
        connect(m_rulesModel, &QAbstractItemModel::rowsRemoved, this, &ClientRecordDialog::updateOverlapWarning);
        connect(m_rulesModel, &QAbstractItemModel::modelReset, this, &ClientRecordDialog::updateOverlapWarning);
        connect(m_rulesModel, &QAbstractItemModel::dataChanged, this, &ClientRecordDialog::updateButtons);
        connect(m_rulesModel, &QAbstractItemModel::modelReset, this, &ClientRecordDialog::updateButtons);

        updateButtons();
        // a warning the dialog opens with is part of how it looks; only one
        // appearing while the rules are edited is worth animating
        m_ui->overlapWarning->setAnimate(false);
        updateOverlapWarning();
        m_ui->overlapWarning->setAnimate(true);
    }

    ClientRecordDialog::~ClientRecordDialog() = default;

    ClientRecord ClientRecordDialog::record() const
    {
        return m_record;
    }

    QList<Entry*> ClientRecordDialog::removedEntries() const
    {
        return m_decisionsModel->removedEntries();
    }

    void ClientRecordDialog::updateButtons()
    {
        const auto current = m_ui->conditionsView->currentIndex();
        m_ui->addConditionButton->setEnabled(current.isValid());
        m_ui->removeConditionButton->setEnabled(current.isValid());
        auto hashable = false;
        if (m_rulesModel->isCondition(current)) {
            const auto& cond = m_rulesModel->rules().at(m_rulesModel->ruleRow(current)).conditions.at(current.row());
            hashable = cond.kind == RuleCondition::Kind::Hash && cond.algo == DefaultExeHashAlgo;
        }
        m_ui->hashFromFileButton->setEnabled(hashable);
        m_ui->removeDecisionButton->setEnabled(m_ui->entryDecisionsView->currentIndex().isValid());
    }

    void ClientRecordDialog::updateOverlapWarning()
    {
        ClientRecord edited;
        edited.rules = m_rulesModel->rules();
        QStringList names;
        for (const auto& other : asConst(m_otherRecords)) {
            if (recordsOverlap(edited, other)) {
                names << QStringLiteral("\"%1\"").arg(other.name);
            }
        }
        if (names.isEmpty()) {
            m_ui->overlapWarning->hideMessage();
        } else {
            m_ui->overlapWarning->showMessage(
                tr("These rules can match the same client as %1. Overlaps resolve to the denying record first, "
                   "then the earliest created one.")
                    .arg(names.join(QStringLiteral(", "))),
                MessageWidget::Warning,
                MessageWidget::DisableAutoHide);
        }
    }

    void ClientRecordDialog::hashFromFile()
    {
        const auto current = m_ui->conditionsView->currentIndex();
        if (!m_rulesModel->isCondition(current)) {
            return;
        }
        const auto path = QFileDialog::getOpenFileName(this, tr("Select executable to hash"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            m_ui->overlapWarning->showMessage(tr("Cannot read file %1").arg(path), MessageWidget::Error);
            return;
        }
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(&file);
        m_rulesModel->setData(current.siblingAtColumn(MatchRulesModel::ColumnValue),
                              QString::fromLatin1(hasher.result().toHex()),
                              Qt::EditRole);
    }

    void ClientRecordDialog::accept()
    {
        const auto name = m_ui->nameEdit->text().trimmed();
        const auto& rules = m_rulesModel->rules();

        QString error;
        if (name.isEmpty()) {
            error = tr("Enter a name for the client.");
        } else if (rules.isEmpty()) {
            error = tr("Add at least one rule, otherwise no client will ever match this record.");
        } else {
            static const QRegularExpression hexDigest(QStringLiteral("^[0-9a-f]+$"));
            for (const auto& rule : rules) {
                if (rule.conditions.isEmpty()) {
                    error = tr("Every rule needs at least one condition.");
                    break;
                }
                for (const auto& cond : rule.conditions) {
                    if (cond.value.isEmpty()) {
                        error = tr("Every condition needs a value.");
                        break;
                    }
                    if (cond.kind == RuleCondition::Kind::Hash && !hexDigest.match(cond.value).hasMatch()) {
                        error = tr("Hash values must be hexadecimal digests.");
                        break;
                    }
                }
                if (!error.isEmpty()) {
                    break;
                }
            }
        }
        if (!error.isEmpty()) {
            m_ui->overlapWarning->showMessage(error, MessageWidget::Error);
            return;
        }

        m_record.name = name;
        m_record.rules = rules;
        switch (m_ui->allEntriesCombo->currentIndex()) {
        case AllEntriesAllow:
            m_record.allEntries = AuthDecision::Allowed;
            break;
        case AllEntriesDeny:
            m_record.allEntries = AuthDecision::Denied;
            break;
        default:
            m_record.allEntries = AuthDecision::Undecided;
            break;
        }
        if (m_record.id.isNull()) {
            m_record.id = QUuid::createUuid();
            m_record.created = Clock::currentDateTimeUtc();
        }
        QDialog::accept();
    }
} // namespace FdoSecrets

#include "ClientRecordDialog.moc"
