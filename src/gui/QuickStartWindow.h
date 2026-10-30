#ifndef KEEPASSXC_QUICKSTARTWINDOW_H
#define KEEPASSXC_QUICKSTARTWINDOW_H

#include <QPointer>
#include <QWidget>

class Entry;
class DatabaseWidget;

class QuickStartWindow : public QWidget
{
    Q_OBJECT

public:
    QuickStartWindow(Entry* entry, DatabaseWidget* dbWidget, QWidget* parent = nullptr);

private:
    QPointer<Entry> m_entry;
    QPointer<DatabaseWidget> m_dbWidget;
};

#endif
