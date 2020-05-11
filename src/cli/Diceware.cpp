/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include <cstdlib>
#include <stdio.h>

#include "Diceware.h"

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/PassphraseGenerator.h"

const QCommandLineOption Diceware::WordCountOption =
    QCommandLineOption(QStringList() << "W"
                                     << "words",
                       QObject::tr("Word count for the diceware passphrase."),
                       QObject::tr("count", "CLI parameter"));

const QCommandLineOption Diceware::WordListOption =
    QCommandLineOption(QStringList() << "w"
                                     << "word-list",
                       QObject::tr("Wordlist for the diceware generator.\n[Default: EFF English]"),
                       QObject::tr("path"));

Diceware::Diceware()
{
    name = QString("diceware");
    description = QObject::tr("Generate a new random diceware passphrase.");
    options.append(Diceware::WordCountOption);
    options.append(Diceware::WordListOption);
}

int Diceware::execute(const QStringList& arguments)
{
    QSharedPointer<QCommandLineParser> parser = getCommandLineParser(arguments);
    if (parser.isNull()) {
        return EXIT_FAILURE;
    }

    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    PassphraseGenerator dicewareGenerator;

    QString wordCount = parser->value(Diceware::WordCountOption);
    if (wordCount.isEmpty()) {
        dicewareGenerator.setWordCount(PassphraseGenerator::DefaultWordCount);
    } else if (wordCount.toInt() <= 0) {
        err << QObject::tr("Invalid word count %1").arg(wordCount) << endl;
        return EXIT_FAILURE;
    } else {
        dicewareGenerator.setWordCount(wordCount.toInt());
    }

    QString wordListFile = parser->value(Diceware::WordListOption);
    if (!wordListFile.isEmpty()) {
        dicewareGenerator.setWordList(wordListFile);
    }

    if (!dicewareGenerator.isValid()) {
        // We already validated the word count input so if the generator is invalid, it
        // must be because the word list is too small.
        err << QObject::tr("The word list is too small (< 1000 items)") << endl;
        return EXIT_FAILURE;
    }

    QString password = dicewareGenerator.generatePassphrase();
    out << password << endl;

    return EXIT_SUCCESS;
}
