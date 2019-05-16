/*
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

#ifndef KEEPASSX_PASSPHRASEGENERATOR_H
#define KEEPASSX_PASSPHRASEGENERATOR_H

#include <QFlags>
#include <QString>
#include <QVector>
#include <QtCore/QScopedPointer>
#include "PasswordGenerator.h"

class PassphraseGenerator
{
public:
    PassphraseGenerator();
    Q_DISABLE_COPY(PassphraseGenerator)

    double calculateEntropy(const QString& passphrase) const;
    void setWordCount(int wordCount);
    void setWordList(const QString& path);
    void setDefaultWordList();
    void setWordSeparator(const QString& separator);
    void setWordSeparator(const PasswordGenerator::CharClasses& classes, const QString& exclude);
    void setEnhancementChars(const PasswordGenerator::CharClasses& classes, const QString& exclude);
    void setEnhancementCount(const int count);
    bool isValid() const;

    QString generatePassphrase();

    static constexpr int DefaultWordCount = 7;
    static const char* DefaultSeparator;
    static const char* DefaultWordList;

private:
    double calculateRawEntropy() const;

    int m_wordCount;
    QVector<QString> m_wordlist;

    bool m_useSeparator = true;
    QString m_separator;
    const QScopedPointer<PasswordGenerator> m_separatorGenerator;

    int m_enhancementCount = 0;
    const QScopedPointer<PasswordGenerator> m_enhancementGenerator;

    double m_lastEntropy = 0;
};

#endif // KEEPASSX_PASSPHRASEGENERATOR_H
