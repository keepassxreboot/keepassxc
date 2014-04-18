/**
 ***************************************************************************
 * @file OptionDialog.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QtGui/QWidget>

namespace Ui {
class OptionDialog;
}

class OptionDialog : public QWidget
{
    Q_OBJECT

public:
    explicit OptionDialog(QWidget *parent = 0);
    ~OptionDialog();

public Q_SLOTS:
    void loadSettings();
    void saveSettings();

Q_SIGNALS:
    void removeSharedEncryptionKeys();
    void removeStoredPermissions();

private:
    Ui::OptionDialog *ui;
};

#endif // OPTIONDIALOG_H
