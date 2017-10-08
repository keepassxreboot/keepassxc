/*
 *  Copyright (C) 2017 Serhii Moroz <frostasm@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSX_EDITWIDGETAUTOTYPE_H
#define KEEPASSX_EDITWIDGETAUTOTYPE_H

#include <QWidget>

#include "core/TimeInfo.h"
#include "core/Uuid.h"


class AutoTypeAssociations;
class AutoTypeAssociationsModel;

namespace Tools {
enum class TriState;
}


class QButtonGroup;

namespace Ui {
    class EditWidgetAutoType;
}

class EditWidgetAutoType : public QWidget
{
    Q_OBJECT

public:
    explicit EditWidgetAutoType(QWidget* parent = nullptr);
    ~EditWidgetAutoType();

    Tools::TriState autoTypeEnabled() const;
    bool inheritSequenceEnabled() const;
    QString sequence() const;
    bool useParentAssociations() const;
    AutoTypeAssociations* autoTypeAssociations();
    const AutoTypeAssociations* autoTypeAssociations() const;

public slots:
    void setFields(const Tools::TriState autoTypeEnabled, const bool parentAutoTypeEnabled,
                   const QString &defaultAutoTypeSequence, const QString &effectiveAutoTypeSequence,
                   const AutoTypeAssociations* autoTypeAssociations);
    void setUseParentAssociations(bool useParentAssociations);
    void setHistory(bool history);
    void clear();
    void removeEmptyAssocs();

private slots:
    void updateAutoTypeEnabled();
    void insertAutoTypeAssoc();
    void removeAutoTypeAssoc();
    void loadCurrentAssoc(const QModelIndex& current);
    void clearCurrentAssoc();
    void applyCurrentAssoc();

private:
    const QScopedPointer<Ui::EditWidgetAutoType> m_ui;

    bool m_history;
    bool m_parentAutoTypeEnabled;

    AutoTypeAssociations* const m_autoTypeAssoc;
    AutoTypeAssociationsModel* const m_autoTypeAssocModel;
    QButtonGroup* const m_defaultSequenceGroup;
    QButtonGroup* const m_windowSequenceGroup;

    Q_DISABLE_COPY(EditWidgetAutoType)
};

#endif // KEEPASSX_EDITWIDGETAUTOTYPE_H
