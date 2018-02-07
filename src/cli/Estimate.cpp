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

#include <QCommandLineParser>
#include <QTextStream>

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
    double e;
    int len = strlen(pwd);
    if (!advanced) {
        e = ZxcvbnMatch(pwd, 0, 0);
        printf("Length %d\tEntropy %.3f\tLog10 %.3f\n", len, e, e * 0.301029996);
    } else {
        int ChkLen;
        ZxcMatch_t *info, *p;
        double m = 0.0;
        e = ZxcvbnMatch(pwd, 0, &info);
        for (p = info; p; p = p->Next) {
            m += p->Entrpy;
        }
        m = e - m;
        printf("Length %d\tEntropy %.3f\tLog10 %.3f\n  Multi-word extra bits %.1f\n", len, e, e * 0.301029996, m);
        p = info;
        ChkLen = 0;
        while (p) {
            int n;
            switch (static_cast<int>(p->Type)) {
            case BRUTE_MATCH:
                printf("  Type: Bruteforce     ");
                break;
            case DICTIONARY_MATCH:
                printf("  Type: Dictionary     ");
                break;
            case DICT_LEET_MATCH:
                printf("  Type: Dict+Leet      ");
                break;
            case USER_MATCH:
                printf("  Type: User Words     ");
                break;
            case USER_LEET_MATCH:
                printf("  Type: User+Leet      ");
                break;
            case REPEATS_MATCH:
                printf("  Type: Repeated       ");
                break;
            case SEQUENCE_MATCH:
                printf("  Type: Sequence       ");
                break;
            case SPATIAL_MATCH:
                printf("  Type: Spatial        ");
                break;
            case DATE_MATCH:
                printf("  Type: Date           ");
                break;
            case BRUTE_MATCH + MULTIPLE_MATCH:
                printf("  Type: Bruteforce(Rep)");
                break;
            case DICTIONARY_MATCH + MULTIPLE_MATCH:
                printf("  Type: Dictionary(Rep)");
                break;
            case DICT_LEET_MATCH + MULTIPLE_MATCH:
                printf("  Type: Dict+Leet(Rep) ");
                break;
            case USER_MATCH + MULTIPLE_MATCH:
                printf("  Type: User Words(Rep)");
                break;
            case USER_LEET_MATCH + MULTIPLE_MATCH:
                printf("  Type: User+Leet(Rep) ");
                break;
            case REPEATS_MATCH + MULTIPLE_MATCH:
                printf("  Type: Repeated(Rep)  ");
                break;
            case SEQUENCE_MATCH + MULTIPLE_MATCH:
                printf("  Type: Sequence(Rep)  ");
                break;
            case SPATIAL_MATCH + MULTIPLE_MATCH:
                printf("  Type: Spatial(Rep)   ");
                break;
            case DATE_MATCH + MULTIPLE_MATCH:
                printf("  Type: Date(Rep)      ");
                break;

            default:
                printf("  Type: Unknown%d ", p->Type);
                break;
            }
            ChkLen += p->Length;
            printf("  Length %d  Entropy %6.3f (%.2f) ", p->Length, p->Entrpy, p->Entrpy * 0.301029996);
            for (n = 0; n < p->Length; ++n, ++pwd) {
                printf("%c", *pwd);
            }
            printf("\n");
            p = p->Next;
        }
        ZxcvbnFreeInfo(info);
        if (ChkLen != len) {
            printf("*** Password length (%d) != sum of length of parts (%d) ***\n", len, ChkLen);
        }
    }
}

int Estimate::execute(const QStringList& arguments)
{
    QTextStream inputTextStream(stdin, QIODevice::ReadOnly);
    QTextStream outputTextStream(stdout, QIODevice::WriteOnly);

    QCommandLineParser parser;
    parser.setApplicationDescription(this->description);
    parser.addPositionalArgument("password", QObject::tr("Password for which to estimate the entropy."), "[password]");
    QCommandLineOption advancedOption(QStringList() << "a"
                                                    << "advanced",
                                      QObject::tr("Perform advanced analysis on the password."));
    parser.addOption(advancedOption);
    parser.process(arguments);

    const QStringList args = parser.positionalArguments();
    if (args.size() > 1) {
        outputTextStream << parser.helpText().replace("keepassxc-cli", "keepassxc-cli estimate");
        return EXIT_FAILURE;
    }

    QString password;
    if (args.size() == 1) {
        password = args.at(0);
    } else {
        password = inputTextStream.readLine();
    }

    estimate(password.toLatin1(), parser.isSet(advancedOption));
    return EXIT_SUCCESS;
}
