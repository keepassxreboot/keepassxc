/**
 ***************************************************************************
 * @file AccessControlDialog.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef ACCESSCONTROLDIALOG_H
#define ACCESSCONTROLDIALOG_H

#include <QtGui/QDialog>

class Entry;

namespace Ui {
class AccessControlDialog;
}

class AccessControlDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AccessControlDialog(QWidget *parent = 0);
    ~AccessControlDialog();

    void setUrl(const QString & url);
    void setItems(const QList<Entry *> & items);
    bool remember() const;
    void setRemember(bool r);
    
private:
    Ui::AccessControlDialog *ui;
};

#endif // ACCESSCONTROLDIALOG_H
