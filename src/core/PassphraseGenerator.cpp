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

#include "PassphraseGenerator.h"

#include <QFile>
#include <QTextStream>

#include "crypto/Random.h"
#include "core/FilePath.h"

PassphraseGenerator::PassphraseGenerator()
    : m_length(0)
{
    const QString path = filePath()->dataPath("wordlists/eff_large.wordlist");

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Couldn't load passphrase wordlist.");
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        m_wordlist.append(in.readLine());
    }
}

void PassphraseGenerator::setLength(int length)
{
    m_length = length;
}

QString PassphraseGenerator::generatePassphrase() const
{
    Q_ASSERT(isValid());

    // In case there was an error loading the wordlist
    if(m_wordlist.length() == 0) {
        QString empty;
        return empty;
    }

    QString passphrase;
    for (int i = 0; i < m_length; i ++) {
      int word_index = randomGen()->randomUInt(m_wordlist.length());
      passphrase.append(m_wordlist.at(word_index));

      if(i < m_length - 1) {
        passphrase.append(" ");
      }
    }

    return passphrase;
}

bool PassphraseGenerator::isValid() const
{
    if (m_length == 0) {
        return false;
    }

    return true;
}
