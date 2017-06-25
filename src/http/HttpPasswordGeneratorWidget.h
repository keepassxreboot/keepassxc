/*
*  Copyright (C) 2013 Francois Ferrand
*  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_HTTPPASSWORDGENERATORWIDGET_H
#define KEEPASSX_HTTPPASSWORDGENERATORWIDGET_H

#include <QWidget>
#include <QScopedPointer>

#include "core/PasswordGenerator.h"

namespace Ui {
    class HttpPasswordGeneratorWidget;
}

class HttpPasswordGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HttpPasswordGeneratorWidget(QWidget* parent = nullptr);
    ~HttpPasswordGeneratorWidget();
    void loadSettings();
    void saveSettings();
    void reset();

private slots:
    void sliderMoved();
    void spinBoxChanged();

    void updateGenerator();

private:
    bool m_updatingSpinBox;

    PasswordGenerator::CharClasses charClasses();
    PasswordGenerator::GeneratorFlags generatorFlags();

    const QScopedPointer<PasswordGenerator> m_generator;
    const QScopedPointer<Ui::HttpPasswordGeneratorWidget> m_ui;
};

#endif // KEEPASSX_PASSWORDGENERATORWIDGET_H
