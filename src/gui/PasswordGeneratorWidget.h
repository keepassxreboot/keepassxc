/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_PASSWORDGENERATORWIDGET_H
#define KEEPASSX_PASSWORDGENERATORWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QWidget>

#include "core/PassphraseGenerator.h"
#include "core/PasswordGenerator.h"

namespace Ui
{
    class PasswordGeneratorWidget;
}

class PasswordGenerator;
class PassphraseGenerator;

class PasswordGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    enum GeneratorTypes
    {
        Password = 0,
        Diceware = 1
    };
    explicit PasswordGeneratorWidget(QWidget* parent = nullptr);
    ~PasswordGeneratorWidget();
    void loadSettings();
    void saveSettings();
    void reset(int length = 0);
    void setStandaloneMode(bool standalone);
    QString getGeneratedPassword();
    bool isPasswordVisible() const;

protected:
    void showEvent(QShowEvent* event) override;

public slots:
    void regeneratePassword();
    void applyPassword();
    void copyPassword();
    void setPasswordVisible(bool visible);

signals:
    void appliedPassword(const QString& password);
    void dialogTerminated();

private slots:
    void updateButtonsEnabled(const QString& password);
    void updatePasswordStrength(const QString& password);
    void selectSimpleMode();
    void selectAdvancedMode();
    void excludeHexChars();

    void passwordSliderMoved();
    void passwordSpinBoxChanged();
    void dicewareSliderMoved();
    void dicewareSpinBoxChanged();
    void colorStrengthIndicator(double entropy);

    void updateGenerator();

private:
    bool m_updatingSpinBox;
    bool m_standalone = false;

    PasswordGenerator::CharClasses charClasses();
    PasswordGenerator::GeneratorFlags generatorFlags();

    const QScopedPointer<PasswordGenerator> m_passwordGenerator;
    const QScopedPointer<PassphraseGenerator> m_dicewareGenerator;
    const QScopedPointer<Ui::PasswordGeneratorWidget> m_ui;

protected:
    void keyPressEvent(QKeyEvent* e) override;
};

#endif // KEEPASSX_PASSWORDGENERATORWIDGET_H
