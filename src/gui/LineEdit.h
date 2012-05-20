/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#ifndef KEEPASSX_LINEEDIT_H
#define KEEPASSX_LINEEDIT_H

#include <QtGui/QLineEdit>

class QToolButton;

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = 0);

protected:
    void resizeEvent(QResizeEvent* event);

private Q_SLOTS:
    void updateCloseButton(const QString& text);

private:
    QToolButton* m_clearButton;
};

#endif // KEEPASSX_LINEEDIT_H
