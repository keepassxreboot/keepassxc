/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include <QCommandLineParser>

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/PassphraseGenerator.h"

Diceware::Diceware()
{
    name = QString("diceware");
    description = QObject::tr("Generate a new random diceware passphrase.");
}

Diceware::~Diceware()
{
}

int Diceware::execute(const QStringList& arguments)
{
    TextStream outputTextStream(Utils::STDOUT, QIODevice::WriteOnly);
    TextStream errorTextStream(Utils::STDERR, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    QCommandLineOption words(QStringList() << "W"
                                           << "words",
                             QObject::tr("Word count for the diceware passphrase."),
                             QObject::tr("count", "CLI parameter"));
    parser.addOption(words);
    QCommandLineOption wordlistFile(QStringList() << "w"
                                                  << "word-list",
                                    QObject::tr("Wordlist for the diceware generator.\n[Default: EFF English]"),
                                    QObject::tr("path"));
    parser.addOption(wordlistFile);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        errorTextStream << parser.helpText().replace("[options]", "diceware [options]");
        return EXIT_FAILURE;
    }

    PassphraseGenerator dicewareGenerator;

    if (parser.value(words).isEmpty()) {
        dicewareGenerator.setWordCount(PassphraseGenerator::DefaultWordCount);
    } else {
        int wordcount = parser.value(words).toInt();
        dicewareGenerator.setWordCount(wordcount);
    }

    if (!parser.value(wordlistFile).isEmpty()) {
        dicewareGenerator.setWordList(parser.value(wordlistFile));
    }

    if (!dicewareGenerator.isValid()) {
        outputTextStream << parser.helpText().replace("[options]", "diceware [options]");
        return EXIT_FAILURE;
    }

    QString password = dicewareGenerator.generatePassphrase();
    outputTextStream << password << endl;

    return EXIT_SUCCESS;
}
