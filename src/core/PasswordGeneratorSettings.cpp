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

#include "PasswordGeneratorSettings.h"
#include "core/Config.h"
#include "core/PasswordHealth.h"

PasswordGeneratorSettings* PasswordGeneratorSettings::m_instance(nullptr);

PasswordGeneratorSettings* PasswordGeneratorSettings::instance()
{
    if (!m_instance) {
        m_instance = new PasswordGeneratorSettings();
    }

    return m_instance;
}

bool PasswordGeneratorSettings::useNumbers()
{
    return config()->get(Config::PasswordGenerator_Numbers).toBool();
}

void PasswordGeneratorSettings::setUseNumbers(bool useNumbers)
{
    config()->set(Config::PasswordGenerator_Numbers, useNumbers);
}

bool PasswordGeneratorSettings::useLowercase()
{
    return config()->get(Config::PasswordGenerator_LowerCase).toBool();
}

void PasswordGeneratorSettings::setUseLowercase(bool useLowercase)
{
    config()->set(Config::PasswordGenerator_LowerCase, useLowercase);
}

bool PasswordGeneratorSettings::useUppercase()
{
    return config()->get(Config::PasswordGenerator_UpperCase).toBool();
}

void PasswordGeneratorSettings::setUseUppercase(bool useUppercase)
{
    config()->set(Config::PasswordGenerator_UpperCase, useUppercase);
}

bool PasswordGeneratorSettings::useSpecial()
{
    return config()->get(Config::PasswordGenerator_SpecialChars).toBool();
}

void PasswordGeneratorSettings::setUseSpecial(bool useSpecial)
{
    config()->set(Config::PasswordGenerator_SpecialChars, useSpecial);
}

bool PasswordGeneratorSettings::useBraces()
{
    return config()->get(Config::PasswordGenerator_Braces).toBool();
}

void PasswordGeneratorSettings::setUseBraces(bool useBraces)
{
    config()->set(Config::PasswordGenerator_Braces, useBraces);
}

bool PasswordGeneratorSettings::usePunctuation()
{
    return config()->get(Config::PasswordGenerator_Punctuation).toBool();
}

void PasswordGeneratorSettings::setUsePunctuation(bool usePunctuation)
{
    config()->set(Config::PasswordGenerator_Punctuation, usePunctuation);
}

bool PasswordGeneratorSettings::useQuotes()
{
    return config()->get(Config::PasswordGenerator_Quotes).toBool();
}

void PasswordGeneratorSettings::setUseQuotes(bool useQuotes)
{
    config()->set(Config::PasswordGenerator_Quotes, useQuotes);
}

bool PasswordGeneratorSettings::useDashes()
{
    return config()->get(Config::PasswordGenerator_Dashes).toBool();
}

void PasswordGeneratorSettings::setUseDashes(bool useDashes)
{
    config()->set(Config::PasswordGenerator_Dashes, useDashes);
}

bool PasswordGeneratorSettings::useMath()
{
    return config()->get(Config::PasswordGenerator_Math).toBool();
}

void PasswordGeneratorSettings::setUseMath(bool useMath)
{
    config()->set(Config::PasswordGenerator_Math, useMath);
}

bool PasswordGeneratorSettings::useLogograms()
{
    return config()->get(Config::PasswordGenerator_Logograms).toBool();
}

void PasswordGeneratorSettings::setUseLogograms(bool useLogograms)
{
    config()->set(Config::PasswordGenerator_Logograms, useLogograms);
}

bool PasswordGeneratorSettings::useEASCII()
{
    return config()->get(Config::PasswordGenerator_EASCII).toBool();
}

void PasswordGeneratorSettings::setUseEASCII(bool useEASCII)
{
    config()->set(Config::PasswordGenerator_EASCII, useEASCII);
}

bool PasswordGeneratorSettings::advancedMode()
{
    return config()->get(Config::PasswordGenerator_AdvancedMode).toBool();
}

void PasswordGeneratorSettings::setAdvancedMode(bool advancedMode)
{
    config()->set(Config::PasswordGenerator_AdvancedMode, advancedMode);
}

QString PasswordGeneratorSettings::additionalChars()
{
    return config()->get(Config::PasswordGenerator_AdditionalChars).toString();
}

void PasswordGeneratorSettings::setAdditionalChars(const QString& chars)
{
    config()->set(Config::PasswordGenerator_AdditionalChars, chars);
}

QString PasswordGeneratorSettings::excludedChars()
{
    return config()->get(Config::PasswordGenerator_ExcludedChars).toString();
}

void PasswordGeneratorSettings::setExcludedChars(const QString& chars)
{
    config()->set(Config::PasswordGenerator_ExcludedChars, chars);
}

int PasswordGeneratorSettings::passPhraseWordCount()
{
    return config()->get(Config::PasswordGenerator_WordCount).toInt();
}

void PasswordGeneratorSettings::setPassPhraseWordCount(int wordCount)
{
    config()->set(Config::PasswordGenerator_WordCount, wordCount);
}

QString PasswordGeneratorSettings::passPhraseWordSeparator()
{
    return config()->get(Config::PasswordGenerator_WordSeparator).toString();
}

void PasswordGeneratorSettings::setPassPhraseWordSeparator(const QString& separator)
{
    config()->set(Config::PasswordGenerator_WordSeparator, separator);
}

QString PasswordGeneratorSettings::passPhraseWordList()
{
    return config()->get(Config::PasswordGenerator_WordList).toString();
}

void PasswordGeneratorSettings::setPassPhraseWordList(const QString& wordList)
{
    config()->set(Config::PasswordGenerator_WordList, wordList);
}

int PasswordGeneratorSettings::generatorType()
{
    return config()->get(Config::PasswordGenerator_Type).toInt();
}

void PasswordGeneratorSettings::setGeneratorType(int type)
{
    config()->set(Config::PasswordGenerator_Type, type);
}

bool PasswordGeneratorSettings::everyGroup()
{
    return config()->get(Config::PasswordGenerator_EnsureEvery).toBool();
}

void PasswordGeneratorSettings::setEveryGroup(bool everyGroup)
{
    config()->set(Config::PasswordGenerator_EnsureEvery, everyGroup);
}

bool PasswordGeneratorSettings::excludeAlike()
{
    return config()->get(Config::PasswordGenerator_ExcludeAlike).toBool();
}

void PasswordGeneratorSettings::setExcludeAlike(bool excludeAlike)
{
    config()->set(Config::PasswordGenerator_ExcludeAlike, excludeAlike);
}

int PasswordGeneratorSettings::length()
{
    return config()->get(Config::PasswordGenerator_Length).toInt();
}

void PasswordGeneratorSettings::setLength(int length)
{
    config()->set(Config::PasswordGenerator_Length, length);
}

PassphraseGenerator::PassphraseWordCase PasswordGeneratorSettings::passPhraseWordCase()
{
    return static_cast<PassphraseGenerator::PassphraseWordCase>(
        config()->get(Config::PasswordGenerator_WordCase).toInt());
}

void PasswordGeneratorSettings::setPassPhraseWordCase(int wordCase)
{
    config()->set(Config::PasswordGenerator_WordCase, wordCase);
}

PasswordGenerator::CharClasses PasswordGeneratorSettings::charClasses()
{
    PasswordGenerator::CharClasses classes;
    if (useLowercase()) {
        classes |= PasswordGenerator::LowerLetters;
    }

    if (useUppercase()) {
        classes |= PasswordGenerator::UpperLetters;
    }

    if (useNumbers()) {
        classes |= PasswordGenerator::Numbers;
    }

    if (useEASCII()) {
        classes |= PasswordGenerator::EASCII;
    }

    if (!advancedMode()) {
        if (useSpecial()) {
            classes |= PasswordGenerator::SpecialCharacters;
        }
    } else {
        if (useBraces()) {
            classes |= PasswordGenerator::Braces;
        }

        if (usePunctuation()) {
            classes |= PasswordGenerator::Punctuation;
        }

        if (useQuotes()) {
            classes |= PasswordGenerator::Quotes;
        }

        if (useDashes()) {
            classes |= PasswordGenerator::Dashes;
        }

        if (useMath()) {
            classes |= PasswordGenerator::Math;
        }

        if (useLogograms()) {
            classes |= PasswordGenerator::Logograms;
        }
    }

    return classes;
}

PasswordGenerator::GeneratorFlags PasswordGeneratorSettings::generatorFlags()
{
    PasswordGenerator::GeneratorFlags flags;
    if (excludeAlike()) {
        flags |= PasswordGenerator::ExcludeLookAlike;
    }
    if (everyGroup()) {
        flags |= PasswordGenerator::CharFromEveryGroup;
    }

    return flags;
}
