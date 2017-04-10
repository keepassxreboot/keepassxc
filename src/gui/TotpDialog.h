#ifndef KEEPASSX_TOTPDIALOG_H
#define KEEPASSX_TOTPDIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include "core/Entry.h"
#include "core/Database.h"
#include "gui/DatabaseWidget.h"

namespace Ui {
    class TotpDialog;
}

class TotpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TotpDialog(DatabaseWidget* parent = nullptr, Entry* entry = nullptr);
    ~TotpDialog();

private:
    unsigned uCounter;
    QScopedPointer<Ui::TotpDialog> m_ui;

private Q_SLOTS:
    void updateTotp();
    void updateProgressBar();
    void updateSeconds();
    void copyToClipboard();
    unsigned resetCounter();

protected:
    Entry* m_entry;
    DatabaseWidget* m_parent;
};

#endif // KEEPASSX_TOTPDIALOG_H
