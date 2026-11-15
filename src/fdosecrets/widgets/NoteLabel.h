/*
 *  Copyright (C) 2026 Aetf <aetf@unlimitedcodeworks.xyz>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_FDOSECRETS_NOTELABEL_H
#define KEEPASSXC_FDOSECRETS_NOTELABEL_H

#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class NoteLabel;
}

/**
 * One line of rich text behind an icon: an information icon, or a warning
 * icon when the note is a warning.
 */
class NoteLabel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool warning READ isWarning WRITE setWarning)

public:
    explicit NoteLabel(QWidget* parent = nullptr);
    ~NoteLabel() override;

    QString text() const;
    void setText(const QString& text);

    bool isWarning() const;
    void setWarning(bool warning);

private:
    void updateIcon();

    QScopedPointer<Ui::NoteLabel> m_ui;
    bool m_warning = false;
};

#endif // KEEPASSXC_FDOSECRETS_NOTELABEL_H
