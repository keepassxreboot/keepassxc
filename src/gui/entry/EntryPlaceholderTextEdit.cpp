#include "EntryPlaceholderTextEdit.h"

#include <QCoreApplication>
#include <QtGlobal>
#include <QKeyEvent>
#include <QMenu>
#include <QListWidget>
#include <QStringList>
#include <QRegularExpression>

#include "core/Entry.h"
#include "gui/Icons.h"


EntryPlaceholder::EntryPlaceholder(const QString& placeholder)
    : m_fullText(placeholder)
{
    QString ph = placeholder;
    // Remove brackets
    if (!ph.isEmpty())
    {
        if (ph[0]=='{') {
            ph = ph.mid(1);
        }
        if (ph[ph.size()-1]=='}') {
            ph = ph.mid(0, ph.size()-1);
        }
    }
    // Parse placeholder parts
    const QStringList parts = ph.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    const int sz = parts.size();
    if (sz==1) {
        // simple placeholder
        m_value = parts[0];
    }
    else if (sz==2) {
        // key-value pair placeholder (URL or S placeholders)
        m_key = parts[0];
        m_value = parts[1];
    }
    else if (sz==3) {
        // REF placeholder only
        m_key = parts[0];
        m_value = parts[2];
        const QStringList crossRef = parts[1].split(QLatin1Char('@'), Qt::SkipEmptyParts);
        if (crossRef.size()==2) {
            m_crossReferenceField = crossRef[0];
            m_crossReferenceSearchIn = crossRef[1];
        }
    }
}

// Colorize the placeholder using Qt HTML format:
// * set to bold if valid
// * set to RED colour if valid but the target value is empty
// * not formatted if invalid
QString EntryPlaceholder::getStyledString(const bool emptyTargetValue) const
{
    QString htmlTag{"<"};
    htmlTag.append(emptyTargetValue ? "span" : "b").append(emptyTargetValue ? " style=\"color:red\"" : "").append(">");
    htmlTag.append("{");

    if (m_key.isEmpty()) {
        htmlTag.append(m_value);
    }
    else if (m_key=="S" || m_key=="URL") {
        htmlTag.append(m_key+":"+m_value);
    }
    else if (m_key=="REF") {
        htmlTag.append(m_key+":"+m_crossReferenceField+"@"+m_crossReferenceSearchIn+":"+m_value);
    }

    htmlTag.append("}").append("</").append(emptyTargetValue ? "span" : "b").append(">");
    return htmlTag;
}

EntryPlaceholderCompletionPopup::EntryPlaceholderCompletionPopup(QWidget* parent)
    : QWidget(parent)
{
    // Setup autocompletion popup
    setAttribute(Qt::WA_QuitOnClose, false);
    setFixedSize(150,200);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setWindowModality(Qt::NonModal);

    QStringList placeholders = Entry::placeholdersList();
    placeholders.append("{REF:}");
    placeholders.append("{S:}");
    m_autoCompletionList = new QListWidget(this);
    for (const QString& placeholder : placeholders) {
        const QString phName = placeholder.mid(1, placeholder.size()-2);
        m_autoCompletionPlaceholdersList.append(phName);
        new QListWidgetItem(phName, m_autoCompletionList);
    }
    m_autoCompletionList->setCurrentRow(0);
    m_autoCompletionList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_autoCompletionList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(m_autoCompletionList, &QListWidget::itemActivated, this, &EntryPlaceholderCompletionPopup::itemActivated);
    connect(m_autoCompletionList, &QListWidget::itemClicked, this, &EntryPlaceholderCompletionPopup::itemClicked);
}

// triggers when a placeholder in the auto-complete popup is selected (with keyboard, in most cases with ENTER key)
void EntryPlaceholderCompletionPopup::itemActivated(QListWidgetItem* item)
{
    emit autoCompletionChoiceSelected(item->text());
}

// triggers when a placeholder in the auto-complete popup is selected (with mouse click)
void EntryPlaceholderCompletionPopup::itemClicked(QListWidgetItem* item)
{
    emit autoCompletionChoiceSelected(item->text());
}

void EntryPlaceholderCompletionPopup::populateCompletionChoices(const QString& filter)
{
    if (isHidden()) return;
    m_autoCompletionList->clear();

    if (!filter.endsWith(":")) {
        for (const QString& placeholder : m_autoCompletionPlaceholdersList) {
            if (filter.isEmpty() || filter=="{" || placeholder.startsWith(filter.mid(1))) {
                new QListWidgetItem(placeholder, m_autoCompletionList);
            }
        }
    }
    if (count()==0) {
        hide();
    }
}

int EntryPlaceholderCompletionPopup::count() const
{
    return m_autoCompletionList->count();
}

int EntryPlaceholderCompletionPopup::getCurrentItemIndex() const
{
    return m_autoCompletionList->currentRow();
}

QString EntryPlaceholderCompletionPopup::getCurrentItemText() const
{
    const auto item = m_autoCompletionList->currentItem();
    return item ? item->text() : "";
}

void EntryPlaceholderCompletionPopup::setCurrentIndex(const int idx) const
{
    m_autoCompletionList->setCurrentRow(idx);
}

// returns whether the placeholder under the cursor matches any item in the auto-complete list
bool EntryPlaceholderCompletionPopup::matchAutoComplete(const QString& placeholder) const
{
    for (const QString& item : m_autoCompletionPlaceholdersList) {
        if (item.startsWith(placeholder)) {
            return true;
        }
    }
    return false;
}

// Custom text edit to display HTML colored text
EntryPlaceholderTextEdit::EntryPlaceholderTextEdit(QWidget* parent)
    : QTextEdit(parent)
    , m_entry(nullptr)
    , m_autoCompletionPopup(new EntryPlaceholderCompletionPopup(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setTabChangesFocus(true);
    setFixedHeight(32);
    viewport()->setCursor(Qt::IBeamCursor);

    connect(this, SIGNAL(cursorPositionChanged()), SLOT(trackCursorPosition()));
    connect(m_autoCompletionPopup, &EntryPlaceholderCompletionPopup::autoCompletionChoiceSelected, this, &EntryPlaceholderTextEdit::applyAutoCompletion);
}

// triggers each time the cursor position changes (not the mouse cursor position, the TEXT cursor position)
void EntryPlaceholderTextEdit::trackCursorPosition()
{
    const int pos = textCursor().position();
    const QString txtBefore = toPlainText().mid(0, pos);
    const QString txtAfter = toPlainText().mid(pos, -1);
    const QString placeholderPreCursor = getPlaceholderBeforeCursor();
    QChar lastCharBefore = txtBefore.isEmpty() ? QChar() : txtBefore[txtBefore.size()-1];
    QChar firstCharAfter = txtAfter.isEmpty() ? QChar() : txtAfter[0];

    // Check whether the auto-complete popup needs to move, or to be hidden/shown
    if (!txtBefore.isEmpty()) {
        auto pt = cursorRect().bottomLeft();
        pt.setY(pt.y() + 7);
        pt.setX(pt.x() + 5);

        if (lastCharBefore=='{') {
            m_autoCompletionPopup->move( mapToGlobal(pt) );
            m_autoCompletionPopup->show();
        }
        else if (lastCharBefore=='}' || firstCharAfter=='{') {
            m_autoCompletionPopup->hide();
            return;
        }
        else if (placeholderPreCursor.size()>0) {
            const bool phIsValid = m_autoCompletionPopup->matchAutoComplete(placeholderPreCursor.mid(1, -1));
            if (phIsValid) {
                m_autoCompletionPopup->move( mapToGlobal(pt) );
                m_autoCompletionPopup->show();
            }
        }
    }

    m_autoCompletionPopup->populateCompletionChoices(placeholderPreCursor);
}

void EntryPlaceholderTextEdit::applyAutoCompletion(QString placeholderName)
{
    m_autoCompletionPopup->hide();
    const QString placeholderPreCursor = getPlaceholderBeforeCursor();
    QString placeholderPostCursor = getPlaceholderAfterCursor();
    QString placeholder = placeholderName.append("}");
    const bool replaceExistingPlaceholder = !placeholderPostCursor.endsWith(" ");
    if (!replaceExistingPlaceholder) {
        placeholderPostCursor.chop(1);
        placeholder.append(" ");
    }

    QTextCursor cur{textCursor()};
    const int pos = cur.position();
    const int startPos = pos + 1 /* for the { char */ - placeholderPreCursor.size();
    const int nbCharsToReplace = placeholderPreCursor.size() + placeholderPostCursor.size();
    const int newCursorPos = pos + 1 /* for the } char */ - placeholderPreCursor.size() + placeholderName.size() + (replaceExistingPlaceholder ? 1 : 0) /* for the extra space */;

    QString txt = toPlainText();
    txt.replace(startPos, nbCharsToReplace, placeholder);
    setText(txt);
    cur.setPosition(newCursorPos);
    setTextCursor(cur);
    redrawPlaceHolders();
}

void EntryPlaceholderTextEdit::setEntry(Entry* e)
{
    if (!e) return;
    m_entry = e;
    setText(e->title());
    redrawPlaceHolders();
}

void EntryPlaceholderTextEdit::keyPressEvent(QKeyEvent* e)
{
    const QString currentAutoCompleteItemText = m_autoCompletionPopup->getCurrentItemText();
    const int currentAutoCompleteItemIndex = m_autoCompletionPopup->getCurrentItemIndex();
    const int key = e->key();

    if (key == Qt::Key_Enter || key == Qt::Key_Return) {
        if (m_autoCompletionPopup->isHidden()) {
            emit accepted();
        }
        else {
            applyAutoCompletion(currentAutoCompleteItemText);
        }
        return;
    }
    if (key == Qt::Key_Up) {
        const int idx = currentAutoCompleteItemIndex;
        if (idx > 0)
        {
            m_autoCompletionPopup->setCurrentIndex(idx-1);
        }
        return;
    }
    if (key == Qt::Key_Down) {
        const int idx = currentAutoCompleteItemIndex;
        if (idx < m_autoCompletionPopup->count()-1)
        {
            m_autoCompletionPopup->setCurrentIndex(idx+1);
        }
        return;
    }
    if (key == Qt::Key_Tab) {
        if (!m_autoCompletionPopup->isHidden())
        {
            applyAutoCompletion(currentAutoCompleteItemText);
        }
        return;
    }
    if (key == Qt::Key_Escape) {
        if (!m_autoCompletionPopup->isHidden())
        {
            m_autoCompletionPopup->hide();
        }
        else {
            emit canceled();
        }
        return;
    }
    QTextEdit::keyPressEvent(e);
    emit textChanged(toPlainText());
    redrawPlaceHolders();
}

// Do not use paintEvent, because we are modifying the text (HTML content)
void EntryPlaceholderTextEdit::redrawPlaceHolders()
{
    if (!m_entry) return;
    QTextCursor cur{textCursor()};
    const int originalPos = cur.position();
    QStringList replacedList;
    QString styledTitle = toPlainText().replace(" ", "&nbsp;"); // 2 consecutive spaces do not display in HTML
    QRegularExpression rx("{[^}{]+}");
    QRegularExpressionMatchIterator i = rx.globalMatch(styledTitle);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        EntryPlaceholder ph{match.captured()};
        const QString placeholder = ph.getString();
        const auto phType = m_entry->placeholderType(placeholder);
        if (phType==Entry::PlaceholderType::NotPlaceholder || phType==Entry::PlaceholderType::Unknown) {
            continue;
        }
        QString substitution = m_entry->resolvePlaceholder(placeholder);
        if (!replacedList.contains(placeholder) && substitution != placeholder) {
            const bool isSubstitutionInvalid = substitution.isEmpty() || substitution=="-1";
            const QString styledPh = ph.getStyledString(isSubstitutionInvalid);
            styledTitle.replace(placeholder, styledPh);
            replacedList << placeholder;
        }
    }
    if (replacedList.size() > 0)
    {
        setHtml(styledTitle);
        cur.setPosition(originalPos);
        setTextCursor(cur);
    }
}

QString EntryPlaceholderTextEdit::getPlaceholderBeforeCursor() const
{
    const int pos = textCursor().position();
    const QString txtBefore = toPlainText().mid(0, pos);

    if (pos>0) {
        for (int i=txtBefore.size()-1; i>=0; --i)
        {
            if (txtBefore[i]=='{')
            {
                return txtBefore.mid(i, pos-1);
            }
        }
    }
    return "";
}

QString EntryPlaceholderTextEdit::getPlaceholderAfterCursor() const
{
    const int pos = textCursor().position();
    const QString txtAfter = toPlainText().mid(pos, -1);

    if (txtAfter.size() > 0) {
        if (txtAfter[0]==" ") {
            return "";
        }
        for (int i=0; i<=txtAfter.size(); ++i)
        {
            if (txtAfter[i]=='{' || txtAfter[i]=='}')
            {
                return txtAfter.mid(0, i);
            }
        }
    }
    return "";
}

