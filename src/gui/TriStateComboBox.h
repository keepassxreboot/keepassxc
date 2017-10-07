#ifndef TRISTATECOMBOBOX_H
#define TRISTATECOMBOBOX_H

#include <QComboBox>

namespace Tools {
enum class TriState;
}

class TriStateComboBox : public QComboBox
{
public:
    explicit TriStateComboBox(QWidget *parent = nullptr);

    Tools::TriState triState() const;

public slots:
    void addTriStateItems(bool inheritValue);
    void setTriState(Tools::TriState triState);
};

#endif // TRISTATECOMBOBOX_H
