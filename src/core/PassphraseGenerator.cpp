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

#include <QFile>
#include <QTextStream>
#include <cmath>

#include "core/FilePath.h"
#include "crypto/Random.h"

const char* PassphraseGenerator::DefaultSeparator = " ";
const char* PassphraseGenerator::DefaultWordList = "eff_large.wordlist";

PassphraseGenerator::PassphraseGenerator()
    : m_wordCount(0)
    , m_separator(PassphraseGenerator::DefaultSeparator)
{
}

double PassphraseGenerator::calculateEntropy(const QString& passphrase)
{
    Q_UNUSED(passphrase);

    if (m_wordlist.isEmpty()) {
        return 0.0;
    }

    return std::log2(m_wordlist.size()) * m_wordCount;
}

void PassphraseGenerator::setWordCount(int wordCount)
{
    if (wordCount > 0) {
        m_wordCount = wordCount;
    } else {
        // safe default if something goes wrong
        m_wordCount = DefaultWordCount;
    }
}

void PassphraseGenerator::setWordList(const QString& path)
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

void PassphraseGenerator::setDefaultWordList()
{
    const QString path = filePath()->wordlistPath(PassphraseGenerator::DefaultWordList);
    setWordList(path);
}

void PassphraseGenerator::setWordSeparator(const QString& separator)
{
    m_separator = separator;
}

QString PassphraseGenerator::generatePassphrase() const
{
    Q_ASSERT(isValid());

    // In case there was an error loading the wordlist
    if (m_wordlist.length() == 0) {
        return QString();
    }

    QStringList words;
    for (int i = 0; i < m_wordCount; ++i) {
        int wordIndex = randomGen()->randomUInt(static_cast<quint32>(m_wordlist.length()));
        words.append(m_wordlist.at(wordIndex));
    }

    return words.join(m_separator);
}

bool PassphraseGenerator::isValid() const
{
    if (m_wordCount == 0) {
        return false;
    }

    return m_wordlist.size() >= 1000;
}
