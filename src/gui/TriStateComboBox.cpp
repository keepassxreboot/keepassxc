#include "TriStateComboBox.h"

#include "core/Tools.h"

TriStateComboBox::TriStateComboBox(QWidget *parent)
    : QComboBox(parent)
{
}

Tools::TriState TriStateComboBox::triState() const
{
    return Tools::triStateFromIndex(currentIndex());
}

void TriStateComboBox::addTriStateItems(bool inheritValue)
{
    clear();
    const QString inheritDefaultString = inheritValue ? tr("Enable")
                                                      : tr("Disable");
    addItem(tr("Inherit from parent group (%1)").arg(inheritDefaultString));
    addItem(tr("Enable"));
    addItem(tr("Disable"));
}

void TriStateComboBox::setTriState(Tools::TriState state)
{
    setCurrentIndex(Tools::indexFromTriState(state));
}
