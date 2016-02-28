/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_PASSWORDGENERATORWIDGET_H
#define KEEPASSX_PASSWORDGENERATORWIDGET_H

#include <QWidget>
#include <QComboBox>

#include "core/PasswordGenerator.h"

namespace Ui {
    class PasswordGeneratorWidget;
}

class PasswordGenerator;

class PasswordGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PasswordGeneratorWidget(QWidget* parent = nullptr);
    ~PasswordGeneratorWidget();
    void loadSettings();
    void reset();

Q_SIGNALS:
    void newPassword(const QString& password);

private Q_SLOTS:
    void updateApplyEnabled(const QString& password);

    void emitNewPassword();
    void saveSettings();
    void sliderMoved();
    void spinBoxChanged();

    void updateGenerator();

private:
    bool m_updatingSpinBox;

    PasswordGenerator::CharClasses charClasses();
    PasswordGenerator::GeneratorFlags generatorFlags();

    const QScopedPointer<PasswordGenerator> m_generator;
    const QScopedPointer<Ui::PasswordGeneratorWidget> m_ui;
};

#endif // KEEPASSX_PASSWORDGENERATORWIDGET_H
