/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PASSWORDGENERATORSETTINGS_H
#define PASSWORDGENERATORSETTINGS_H

#include "core/PassphraseGenerator.h"
#include "core/PasswordGenerator.h"

class PasswordGeneratorSettings
{
public:
    explicit PasswordGeneratorSettings() = default;
    static PasswordGeneratorSettings* instance();

    bool useNumbers();
    void setUseNumbers(bool useNumbers);
    bool useLowercase();
    void setUseLowercase(bool useLowercase);
    bool useUppercase();
    void setUseUppercase(bool useUppercase);
    bool useSpecial();
    void setUseSpecial(bool useSpecial);
    bool useBraces();
    void setUseBraces(bool useBraces);
    bool usePunctuation();
    void setUsePunctuation(bool usePunctuation);
    bool useQuotes();
    void setUseQuotes(bool useQuotes);
    bool useDashes();
    void setUseDashes(bool useDashes);
    bool useMath();
    void setUseMath(bool useMath);
    bool useLogograms();
    void setUseLogograms(bool useLogograms);
    bool useEASCII();
    void setUseEASCII(bool useEASCII);
    bool advancedMode();
    void setAdvancedMode(bool advancedMode);
    QString additionalChars();
    void setAdditionalChars(const QString& chars);
    QString excludedChars();
    void setExcludedChars(const QString& chars);
    int passPhraseWordCount();
    void setPassPhraseWordCount(int wordCount);
    QString passPhraseWordSeparator();
    void setPassPhraseWordSeparator(const QString& separator);
    QString passPhraseWordList();
    void setPassPhraseWordList(const QString& wordList);
    int generatorType();
    void setGeneratorType(int type);
    bool everyGroup();
    void setEveryGroup(bool everyGroup);
    bool excludeAlike();
    void setExcludeAlike(bool excludeAlike);
    int length();
    void setLength(int length);
    PassphraseGenerator::PassphraseWordCase passPhraseWordCase();
    void setPassPhraseWordCase(int wordCase);
    PasswordGenerator::CharClasses charClasses();
    PasswordGenerator::GeneratorFlags generatorFlags();

private:
    static PasswordGeneratorSettings* m_instance;
};

inline PasswordGeneratorSettings* passwordGeneratorSettings()
{
    return PasswordGeneratorSettings::instance();
}

#endif // PASSWORDGENERATORSETTINGS_H
