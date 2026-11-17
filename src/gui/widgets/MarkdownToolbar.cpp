/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "MarkdownToolbar.h"

#include "core/Config.h"
#include "gui/Icons.h"
#include "gui/styles/StateColorPalette.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextEdit>
#include <QToolButton>
#include <algorithm>

MarkdownToolbar::MarkdownToolbar(QWidget* parent)
    : QToolBar(parent)
{
    setupUi();
}

void MarkdownToolbar::setTarget(QTextEdit* edit)
{
    m_target = edit;
    if (!m_target)
        return;

    m_target->setContextMenuPolicy(Qt::DefaultContextMenu);

    // Undo
    if (m_undoAction) {
        m_undoAction->disconnect();
        connect(m_undoAction, &QAction::triggered, m_target, &QTextEdit::undo);

        m_undoAction->setEnabled(m_target->document()->isUndoAvailable());

        connect(m_target->document(), &QTextDocument::undoAvailable, m_undoAction, &QAction::setEnabled);
    }

    // Redo
    if (m_redoAction) {
        m_redoAction->disconnect();
        connect(m_redoAction, &QAction::triggered, m_target, &QTextEdit::redo);

        m_redoAction->setEnabled(m_target->document()->isRedoAvailable());

        connect(m_target->document(), &QTextDocument::redoAvailable, m_redoAction, &QAction::setEnabled);
    }
}

QAction* MarkdownToolbar::createAction(const QString& iconName,
                                       const QString& tooltip,
                                       const QKeySequence& shortcut,
                                       const std::function<void()>& callback)
{
    QAction* act = addAction(icons()->icon(iconName), tooltip);

    if (!shortcut.isEmpty()) {
        act->setShortcut(shortcut);
        QString tip = tooltip;
        tip += " (" + shortcut.toString(QKeySequence::NativeText) + ")";
        act->setToolTip(tip);
    } else {
        act->setToolTip(tooltip);
    }

    act->setShortcutVisibleInContextMenu(true);

    if (auto tb = qobject_cast<QToolButton*>(widgetForAction(act)))
        tb->setAutoRaise(true);

    if (callback)
        connect(act, &QAction::triggered, this, callback);

    return act;
}

void MarkdownToolbar::setupUi()
{
    setMovable(false);
    setFloatable(false);
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    if (config()->get(Config::GUI_CompactMode).toBool())
        setIconSize(QSize(20, 20));

    // Undo
    QString undoText = tr("Undo");
    QString undoShortcut = QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText);
    m_undoAction = createAction("undo", undoText, QKeySequence(), [=]() {
        if (m_target)
            m_target->undo();
    });
    m_undoAction->setToolTip(QString("%1 (%2)").arg(undoText, undoShortcut));

    // Redo
    QString redoText = tr("Redo");
    QString redoShortcut = QKeySequence(QKeySequence::Redo).toString(QKeySequence::NativeText);
    m_redoAction = createAction("redo", redoText, QKeySequence(), [=]() {
        if (m_target)
            m_target->redo();
    });
    m_redoAction->setToolTip(QString("%1 (%2)").arg(redoText, redoShortcut));

    addSeparator();

    // Other groups of buttons
    struct ButtonSpec
    {
        QString iconName;
        QString tooltip;
        QKeySequence shortcut;
        std::function<void()> callback;
    };

    const QList<QList<ButtonSpec>> buttonGroups = {
        {
            {"format-header-decrease",
             tr("Decrease heading level"),
             QKeySequence(Qt::CTRL | Qt::Key_Down),
             [=]() { changeHeadingLevel(-1); }},
            {"format-header-increase",
             tr("Increase heading level"),
             QKeySequence(Qt::CTRL | Qt::Key_Up),
             [=]() { changeHeadingLevel(1); }},
        },
        {
            {"format-text-bold", tr("Bold"), QKeySequence::Bold, [=]() { wrapSelection("**", "**"); }},
            {"format-text-italic", tr("Italic"), QKeySequence::Italic, [=]() { wrapSelection("*", "*"); }},
            {"format-text-strikethrough",
             tr("Strike"),
             QKeySequence(Qt::CTRL | Qt::Key_S),
             [=]() { wrapSelection("~~", "~~"); }},
            {"format-text-underline",
             tr("Underline"),
             QKeySequence::Underline,
             [=]() { wrapSelection("<u>", "</u>"); }},
        },
        {
            {"insert-link", tr("Link"), QKeySequence(Qt::CTRL | Qt::Key_L), [=]() { insertLink(); }},
        },
        {
            {"format-list-bulleted",
             tr("Bullet list"),
             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B),
             [=]() { wrapSelection("- ", "", true); }},
            {"format-list-numbered",
             tr("Numbered list"),
             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N),
             [=]() { wrapSelectionWithNumbering(); }},
            {"format-list-check",
             tr("Task list"),
             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T),
             [=]() { wrapSelection("- [ ] ", "", true); }},
        },
        {
            {"format-code", tr("Inline code"), QKeySequence(Qt::CTRL | Qt::Key_E), [=]() { wrapSelection("`", "`"); }},
            {"format-code-block",
             tr("Code block"),
             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E),
             [=]() { wrapSelection("```\n", "\n```"); }},
        }};

    for (int i = 0; i < buttonGroups.size(); ++i) {
        for (const auto& btn : buttonGroups[i]) {
            createAction(btn.iconName, btn.tooltip, btn.shortcut, btn.callback);
        }
        if (i < buttonGroups.size() - 1)
            addSeparator();
    }
}

void MarkdownToolbar::wrapSelection(const QString& prefix, const QString& suffix, bool lineWise)
{
    if (!m_target)
        return;

    QTextCursor cursor = m_target->textCursor();
    QString selected = cursor.selectedText().replace(QChar::ParagraphSeparator, '\n');

    QString result;
    if (lineWise) {
        QStringList lines = selected.split('\n');
        for (QString& line : lines) {
            line = prefix + line + suffix;
        }
        result = lines.join("\n");
    } else {
        result = prefix + selected + suffix;
    }

    cursor.beginEditBlock();
    cursor.insertText(result);
    cursor.endEditBlock();
}

void MarkdownToolbar::wrapSelectionWithNumbering()
{
    if (!m_target)
        return;

    QTextCursor cursor = m_target->textCursor();
    QString selected = cursor.selectedText().replace(QChar::ParagraphSeparator, '\n');

    QStringList lines = selected.split('\n');
    QStringList numbered;
    for (int i = 0; i < lines.size(); ++i) {
        numbered << QString::number(i + 1) + ". " + lines[i];
    }

    cursor.beginEditBlock();
    cursor.insertText(numbered.join("\n"));
    cursor.endEditBlock();
}

void MarkdownToolbar::changeHeadingLevel(int delta, bool clearAll)
{
    if (!m_target)
        return;

    static const QRegularExpression HeadingRegex(R"rx(^(#{1,6})(?:\s+(.*))?$)rx");

    QTextDocument* doc = m_target->document();
    if (!doc)
        return;

    QTextCursor cur = m_target->textCursor();
    const int origAnchor = cur.anchor();
    const int origCaret = cur.position();
    const bool collapsed = (origAnchor == origCaret);

    struct CursorPos
    {
        int blockNumber;
        int offsetInContent;
    };

    auto parseLine = [&](const QString& line, int& oldLevel, QString& pureContent) {
        oldLevel = 0;
        pureContent = line;

        auto hm = HeadingRegex.match(pureContent);
        if (hm.hasMatch()) {
            oldLevel = hm.captured(1).size();
            pureContent = hm.captured(2);
        }
    };

    auto toCursorPos = [&](int pos) -> CursorPos {
        QTextBlock b = doc->findBlock(pos);
        if (!b.isValid()) {
            return CursorPos{doc->lastBlock().blockNumber(), 0};
        }
        const QString text = b.text();
        int ol = 0;
        QString pure;
        parseLine(text, ol, pure);
        const int prefixLen = text.length() - pure.length();
        const int offInBlock = pos - b.position();
        const int offInContent = qBound(0, offInBlock - prefixLen, pure.length());
        return CursorPos{b.blockNumber(), offInContent};
    };

    auto fromCursorPos = [&](const CursorPos& cp) -> int {
        QTextBlock b = doc->findBlockByNumber(cp.blockNumber);
        if (!b.isValid()) {
            return qMax(0, doc->characterCount() - 1);
        }
        const QString text = b.text();
        int ol = 0;
        QString pure;
        parseLine(text, ol, pure);
        const int prefixLen = text.length() - pure.length();
        const int offInContent = qBound(0, cp.offsetInContent, pure.length());
        return b.position() + prefixLen + offInContent;
    };

    const bool anchorWasEnd = (origAnchor > origCaret);
    CursorPos anchorPos = toCursorPos(origAnchor);
    CursorPos caretPos = toCursorPos(origCaret);

    const int selStart = qMin(origAnchor, origCaret);
    const int selEnd = qMax(origAnchor, origCaret);

    QVector<QTextBlock> blocks;
    if (collapsed) {
        QTextBlock b = doc->findBlock(selStart);
        if (b.isValid())
            blocks.push_back(b);
    } else {
        for (QTextBlock b = doc->findBlock(selStart); b.isValid(); b = b.next()) {
            if (b.position() <= selEnd) {
                blocks.push_back(b);
            } else {
                break;
            }
        }
    }
    if (blocks.isEmpty())
        return;

    bool hasHeadings = false;
    for (const QTextBlock& b : blocks) {
        int ol = 0;
        QString pc;
        parseLine(b.text(), ol, pc);
        if (ol > 0) {
            hasHeadings = true;
            break;
        }
    }

    int targetLevel = -1;
    if (clearAll) {
        targetLevel = 0;
    } else if (!hasHeadings) {
        targetLevel = std::clamp(delta, 0, 6);
    }

    cur.beginEditBlock();

    for (const QTextBlock& b : blocks) {
        const QString original = b.text();
        int oldLevel = 0;
        QString pure;
        parseLine(original, oldLevel, pure);

        const int newLevel = (targetLevel >= 0) ? targetLevel : std::clamp(oldLevel + delta, 0, 6);

        const QString hashes = (newLevel == 0) ? QString() : QString(newLevel, QChar('#'));
        const QString space = (newLevel == 0 || pure.isEmpty()) ? QString() : QStringLiteral(" ");
        const QString newLine = (newLevel == 0) ? pure : hashes + space + pure;

        if (newLine != original) {
            QTextCursor lc(b);
            lc.movePosition(QTextCursor::StartOfBlock);
            lc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            lc.insertText(newLine);
        }
    }

    cur.endEditBlock();

    // Restore cursor
    QTextCursor nc = m_target->textCursor();
    if (collapsed) {
        nc.setPosition(fromCursorPos(caretPos));
    } else {
        const int start = fromCursorPos(anchorPos);
        const int end = fromCursorPos(caretPos);
        if (anchorWasEnd) {
            nc.setPosition(end);
            nc.setPosition(start, QTextCursor::KeepAnchor);
        } else {
            nc.setPosition(start);
            nc.setPosition(end, QTextCursor::KeepAnchor);
        }
    }
    m_target->setTextCursor(nc);
}

void MarkdownToolbar::insertLink()
{
    if (!m_target)
        return;

    QTextCursor cursor = m_target->textCursor();
    QString originalSelected = cursor.selectedText().replace(QChar::ParagraphSeparator, '\n');
    QString trimmedSelected = originalSelected.trimmed();

    QString prefillText, prefillUrl, prefillTooltip;
    bool hasSelection = !trimmedSelected.isEmpty();
    bool matched = false;

    auto tryMatch = [&](const QRegularExpression& re, auto handler) {
        auto m = re.match(trimmedSelected);
        if (m.hasMatch()) {
            matched = true;
            handler(m);
        }
    };

    static const QRegularExpression PlainUrlRegex(R"rx(^(https?://\S+)$)rx", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression EmailRegex(R"rx(^([A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,})$)rx");

    tryMatch(PlainUrlRegex, [&](const auto& m) {
        prefillUrl = m.captured(1);
        prefillText = prefillUrl;
        prefillText.remove(QRegularExpression("^https?://", QRegularExpression::CaseInsensitiveOption));
    });

    if (!matched)
        tryMatch(EmailRegex, [&](const auto& m) {
            prefillUrl = "mailto:" + m.captured(1);
            prefillText = m.captured(1);
        });

    if (hasSelection && !matched) {
        prefillText = trimmedSelected;
        cursor.removeSelectedText();
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Insert Link"));
    dlg.resize(420, 160);
    QFormLayout layout(&dlg);
    layout.setContentsMargins(12, 12, 12, 12);
    layout.setVerticalSpacing(10);

    QLineEdit urlEdit;
    urlEdit.setPlaceholderText("https://example.com");
    urlEdit.setToolTip(tr("Full link address (e.g. https://example.com or mailto:user@example.com)"));

    if (!prefillUrl.isEmpty())
        urlEdit.setText(prefillUrl);
    layout.addRow("URL:", &urlEdit);

    QLineEdit textEdit;
    textEdit.setToolTip(tr("Text shown instead of the URL"));
    if (!prefillText.isEmpty())
        textEdit.setText(prefillText);
    layout.addRow(tr("Link text:"), &textEdit);

    QLineEdit tooltipEdit;
    tooltipEdit.setToolTip(tr("Text shown on hover"));
    if (!prefillTooltip.isEmpty())
        tooltipEdit.setText(prefillTooltip);
    layout.addRow(tr("Tooltip:"), &tooltipEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto okBtn = buttons.button(QDialogButtonBox::Ok);
    okBtn->setAutoDefault(false);
    okBtn->setDefault(false);
    layout.addRow(&buttons);

    StateColorPalette statePalette;
    auto errorColor = statePalette.color(StateColorPalette::ColorRole::Error);

    connect(&urlEdit, &QLineEdit::textChanged, [&](const QString& text) {
        if (!text.trimmed().isEmpty()) {
            urlEdit.setStyleSheet("");
        }
    });

    auto tryAccept = [&]() {
        if (urlEdit.text().trimmed().isEmpty()) {
            urlEdit.setStyleSheet(QString("QLineEdit { background: %1; }").arg(errorColor.name()));
            urlEdit.setFocus();
            return;
        }
        dlg.accept();
    };

    connect(&urlEdit, &QLineEdit::returnPressed, tryAccept);
    connect(&textEdit, &QLineEdit::returnPressed, tryAccept);
    connect(&tooltipEdit, &QLineEdit::returnPressed, tryAccept);
    connect(okBtn, &QPushButton::clicked, tryAccept);
    connect(buttons.button(QDialogButtonBox::Cancel), &QPushButton::clicked, &dlg, &QDialog::reject);

    urlEdit.setFocus();

    if (dlg.exec() != QDialog::Accepted) {
        if (hasSelection) {
            cursor.insertText(originalSelected);
        }
        return;
    }

    const QString url = urlEdit.text().trimmed();
    const QString text = textEdit.text().trimmed().isEmpty() ? url : textEdit.text().trimmed();
    const QString tooltip = tooltipEdit.text().trimmed();

    QString link = tooltip.isEmpty() ? QStringLiteral("[%1](%2)").arg(text, url)
                                     : QStringLiteral("[%1](%2 \"%3\")").arg(text, url, tooltip);

    cursor.insertText(link);
    m_target->setTextCursor(cursor);
}
