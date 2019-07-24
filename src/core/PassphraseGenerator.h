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
#include <QMetaType>

class PassphraseGenerator
{
public:
    PassphraseGenerator();
    Q_DISABLE_COPY(PassphraseGenerator)

    enum WordCaseOption
    {
        Lower,
        Upper,
        Capital
    };

    double calculateEntropy(const QString& passphrase);
    void setWordCount(int wordCount);
    void setWordList(const QString& path);
    void setDefaultWordList();
    void setWordSeparator(const QString& separator);
    void setWordCase(const WordCaseOption wordCase);
    bool isValid() const;

    QString generatePassphrase() const;

    static constexpr int DefaultWordCount = 7;
    static const char* DefaultSeparator;
    static const char* DefaultWordList;
    static const WordCaseOption DefaultCase;

private:
    int m_wordCount;
    QString m_separator;
    WordCaseOption m_wordcase;
    QVector<QString> m_wordlist;
};

// Declare as metatype so we can store it in the combo box item data
Q_DECLARE_METATYPE(PassphraseGenerator::WordCaseOption)

#endif // KEEPASSX_PASSPHRASEGENERATOR_H
