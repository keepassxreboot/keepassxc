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

class PassphraseGenerator
{
public:
    PassphraseGenerator();

    double calculateEntropy(QString passphrase);
    void setWordCount(int wordCount);
    void setWordList(QString path);
    void setWordSeparator(QString separator);
    bool isValid() const;

    QString generatePassphrase() const;

private:
    int m_wordCount;
    QString m_separator;
    QVector<QString> m_wordlist;

    Q_DISABLE_COPY(PassphraseGenerator)
};

#endif // KEEPASSX_PASSPHRASEGENERATOR_H