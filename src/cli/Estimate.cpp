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

#include "Estimate.h"
#include "cli/Utils.h"

#include <QCommandLineParser>

#include "cli/TextStream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zxcvbn.h>

/* For pre-compiled headers under windows */
#ifdef _WIN32
#ifndef __MINGW32__
#include "stdafx.h"
#endif
#endif

Estimate::Estimate()
{
    name = QString("estimate");
    description = QObject::tr("Estimate the entropy of a password.");
}

Estimate::~Estimate()
{
}

static void estimate(const char* pwd, bool advanced)
{
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);

    double e = 0.0;
    int len = static_cast<int>(strlen(pwd));
    if (!advanced) {
        e = ZxcvbnMatch(pwd, nullptr, nullptr);
        out << QObject::tr("Length %1").arg(len, 0) << '\t'
            << QObject::tr("Entropy %1").arg(e, 0, 'f', 3) << '\t'
            << QObject::tr("Log10 %1").arg(e * 0.301029996, 0, 'f', 3) << endl;
    } else {
        int ChkLen = 0;
        ZxcMatch_t *info, *p;
        double m = 0.0;
        e = ZxcvbnMatch(pwd, nullptr, &info);
        for (p = info; p; p = p->Next) {
            m += p->Entrpy;
        }
        m = e - m;
        out << QObject::tr("Length %1").arg(len) << '\t'
            << QObject::tr("Entropy %1").arg(e, 0, 'f', 3) << '\t'
            << QObject::tr("Log10 %1").arg(e * 0.301029996, 0, 'f', 3) << "\n  "
            << QObject::tr("Multi-word extra bits %1").arg(m, 0, 'f', 1) << endl;
        p = info;
        ChkLen = 0;
        while (p) {
            int n;
            switch (static_cast<int>(p->Type)) {
            case BRUTE_MATCH:
                out << "  " << QObject::tr("Type: Bruteforce") << "       ";
                break;
            case DICTIONARY_MATCH:
                out << "  " << QObject::tr("Type: Dictionary") << "       ";
                break;
            case DICT_LEET_MATCH:
                out << "  " << QObject::tr("Type: Dict+Leet") << "        ";
                break;
            case USER_MATCH:
                out << "  " << QObject::tr("Type: User Words") << "       ";
                break;
            case USER_LEET_MATCH:
                out << "  " << QObject::tr("Type: User+Leet") << "        ";
                break;
            case REPEATS_MATCH:
                out << "  " << QObject::tr("Type: Repeated") << "         ";
                break;
            case SEQUENCE_MATCH:
                out << "  " << QObject::tr("Type: Sequence") << "         ";
                break;
            case SPATIAL_MATCH:
                out << "  " << QObject::tr("Type: Spatial") << "          ";
                break;
            case DATE_MATCH:
                out << "  " << QObject::tr("Type: Date") << "             ";
                break;
            case BRUTE_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Bruteforce(Rep)") << "  ";
                break;
            case DICTIONARY_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Dictionary(Rep)") << "  ";
                break;
            case DICT_LEET_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Dict+Leet(Rep)") << "   ";
                break;
            case USER_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: User Words(Rep)") << "  ";
                break;
            case USER_LEET_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: User+Leet(Rep)") << "   ";
                break;
            case REPEATS_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Repeated(Rep)") << "    ";
                break;
            case SEQUENCE_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Sequence(Rep)") << "    ";
                break;
            case SPATIAL_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Spatial(Rep)") << "     ";
                break;
            case DATE_MATCH + MULTIPLE_MATCH:
                out << "  " << QObject::tr("Type: Date(Rep)") << "        ";
                break;

            default:
                out << "  " << QObject::tr("Type: Unknown%1").arg(p->Type) << "        ";
                break;
            }
            ChkLen += p->Length;

            out << QObject::tr("Length %1").arg(p->Length) << '\t'
                << QObject::tr("Entropy %1 (%2)").arg(p->Entrpy, 6, 'f', 3).arg(p->Entrpy * 0.301029996, 0, 'f', 2) << '\t';
            for (n = 0; n < p->Length; ++n, ++pwd) {
                out << *pwd;
            }
            out << endl;
            p = p->Next;
        }
        ZxcvbnFreeInfo(info);
        if (ChkLen != len) {
            out << QObject::tr("*** Password length (%1) != sum of length of parts (%2) ***").arg(len).arg(ChkLen) << endl;
        }
    }
}

int Estimate::execute(const QStringList& arguments)
{
    TextStream in(Utils::STDIN, QIODevice::ReadOnly);
    TextStream out(Utils::STDOUT, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addPositionalArgument("password", QObject::tr("Password for which to estimate the entropy."), "[password]");
    QCommandLineOption advancedOption(QStringList() << "a" << "advanced",
                                      QObject::tr("Perform advanced analysis on the password."));
    parser.addOption(advancedOption);
    parser.addHelpOption();
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() > 1) {
        out << parser.helpText().replace("keepassxc-cli", "keepassxc-cli estimate");
        return EXIT_FAILURE;
    }

    QString password;
    if (args.size() == 1) {
        password = args.at(0);
    } else {
        password = in.readLine();
    }

    estimate(password.toLatin1(), parser.isSet(advancedOption));
    return EXIT_SUCCESS;
}
