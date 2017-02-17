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

#ifndef KEEPASSX_PASSPHRASEGENERATORWIDGET_H
#define KEEPASSX_PASSPHRASEGENERATORWIDGET_H

#include <QWidget>
#include <QComboBox>

#include "core/PassphraseGenerator.h"

namespace Ui {
    class PassphraseGeneratorWidget;
}

class PassphraseGenerator;

class PassphraseGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PassphraseGeneratorWidget(QWidget* parent = nullptr);
    ~PassphraseGeneratorWidget();
    void loadSettings();
    void reset();
    void regeneratePassphrase();

Q_SIGNALS:
    void newPassword(const QString& password);

private Q_SLOTS:
    void updateApplyEnabled(const QString& password);

    void emitNewPassphrase();
    void saveSettings();
    void sliderMoved();
    void spinBoxChanged();

    void updateGenerator();

private:
    bool m_updatingSpinBox;

    const QScopedPointer<PassphraseGenerator> m_generator;
    const QScopedPointer<Ui::PassphraseGeneratorWidget> m_ui;
};

#endif // KEEPASSX_PASSPHRASEGENERATORWIDGET_H
