#ifndef KEEPASSX_SEARCHEDIT_H
#define KEEPASSX_SEARCHEDIT_H

#include <QLineEdit>

/* namespace Ui { */
/*     class SearchEdit; */
/* } */

class SearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    SearchEdit(QWidget *parent);
    virtual ~SearchEdit() {}

protected:
    void focusInEvent(QFocusEvent *event);
};
#endif // SEARCHEDIT_H
