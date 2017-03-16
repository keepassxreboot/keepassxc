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

#include <QWidget>
#include <QScopedPointer>

namespace Ui {
class OptionDialog;
}

class OptionDialog : public QWidget
{
    Q_OBJECT

public:
    explicit OptionDialog(QWidget *parent = nullptr);
    ~OptionDialog();

public slots:
    void loadSettings();
    void saveSettings();

signals:
    void removeSharedEncryptionKeys();
    void removeStoredPermissions();

private:
    QScopedPointer<Ui::OptionDialog> m_ui;
};

#endif // OPTIONDIALOG_H
