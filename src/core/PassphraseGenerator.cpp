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

#include "PassphraseGenerator.h"

#include <math.h>
#include <QFile>
#include <QTextStream>

#include "crypto/Random.h"
#include "core/FilePath.h"

PassphraseGenerator::PassphraseGenerator()
    : m_wordCount(0)
    , m_separator(' ')
{
    const QString path = filePath()->dataPath("wordlists/eff_large.wordlist");
    setWordList(path);
}

double PassphraseGenerator::calculateEntropy(QString passphrase)
{
    Q_UNUSED(passphrase);

    if (m_wordlist.size() == 0) {
        return 0;
    }

    return log(m_wordlist.size()) / log(2.0) * m_wordCount;
}

void PassphraseGenerator::setWordCount(int wordCount)
{
    if (wordCount > 0) {
        m_wordCount = wordCount; 
    } else {
        // safe default if something goes wrong
        m_wordCount = 7;
    }
 
}

void PassphraseGenerator::setWordList(QString path)
{
    m_wordlist.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Couldn't load passphrase wordlist.");
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        m_wordlist.append(in.readLine());
    }

    if (m_wordlist.size() < 4000) {
        qWarning("Wordlist too short!");
        return;
    }
}

void PassphraseGenerator::setWordSeparator(QString separator) {
    m_separator = separator;
}

QString PassphraseGenerator::generatePassphrase() const
{
    Q_ASSERT(isValid());

    // In case there was an error loading the wordlist
    if(m_wordlist.length() == 0) {
        return QString();
    }

    QStringList words;
    for (int i = 0; i < m_wordCount; i++) {
        int wordIndex = randomGen()->randomUInt(m_wordlist.length());
        words.append(m_wordlist.at(wordIndex));
    }

    return words.join(m_separator);
}

bool PassphraseGenerator::isValid() const
{
    if (m_wordCount == 0) {
       return false;
    }

    if (m_wordlist.size() < 1000) {
        return false;
    }

    return true;
}
