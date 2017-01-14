#include "SearchEdit.h"

// This widget will select the whole text automatically when focused,
// except when focused by a mouse press.
SearchEdit::SearchEdit(QWidget *parent)
    : QLineEdit(parent)
{
}

void SearchEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    selectAll();
    // If this event is triggered by a mouse press, the selection will
    // be lost by the following default mouse press handler.
}
