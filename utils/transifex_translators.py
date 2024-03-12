#!/usr/bin/env python3
import json
import os

# Download Transifex languages dump at: https://www.transifex.com/api/2/project/keepassxc/languages
# Language information from https://www.wikiwand.com/en/List_of_ISO_639-1_codes and http://www.lingoes.net/en/translator/langcode.htm

LANGS = {
    "ar" : "العربية (Arabic)",
    "bn" : "বাংলা (Bengali)",
    "ca" : "català (Catalan)",
    "cs" : "čeština (Czech)",
    "da" : "dansk (Danish)",
    "de" : "Deutsch (German)",
    "el" : "ελληνικά (Greek)",
    "eo" : "Esperanto (Esperanto)",
    "es" : "Español (Spanish)",
    "et" : "eesti (Estonian)",
    "eu" : "euskara (Basque)",
    "fa" : "فارسی (Farsi)",
    "fa_IR" : "فارسی (Farsi (Iran))",
    "fi" : "suomi (Finnish)",
    "fr" : "français (French)",
    "gl" : "Galego (Galician)",
    "he" : "עברית (Hebrew)",
    "hr_HR" : "hrvatski jezik (Croatian)",
    "hu" : "magyar (Hungarian)",
    "id" : "Bahasa Indonesia (Indonesian)",
    "is_IS" : "Íslenska (Icelandic)",
    "it" : "Italiano (Italian)",
    "ja" : "日本語 (Japanese)",
    "kk" : "қазақ тілі (Kazakh)",
    "ko" : "한국어 (Korean)",
    "la" : "latine (Latin)",
    "lt" : "lietuvių kalba (Lithuanian)",
    "lv" : "latviešu valoda (Latvian)",
    "nb" : "Norsk Bokmål (Norwegian Bokmål)",
    "nl_NL" : "Nederlands (Dutch)",
    "my" : "ဗမာစာ (Burmese)",
    "pa" : "ਪੰਜਾਬੀ (Punjabi)",
    "pa_IN" : "ਪੰਜਾਬੀ (Punjabi (India))",
    "pl" : "język polski (Polish)",
    "pt" : "Português (Portuguese)",
    "pt_BR" : "Português (Portuguese (Brazil))",
    "pt_PT" : "Português (Portuguese (Portugal))",
    "ro" : "Română (Romanian)",
    "ru" : "русский (Russian)",
    "sk" : "Slovenčina (Slovak)",
    "sl_SI" : "Slovenščina (Slovenian)",
    "sr" : "српски језик (Serbian)",
    "sv" : "Svenska (Swedish)",
    "th" : "ไทย (Thai)",
    "tr" : "Türkçe (Turkish)",
    "uk" : "Українська (Ukrainian)",
    "zh_CN" : "中文 (Chinese (Simplified))",
    "zh_TW" : "中文 (台灣) (Chinese (Traditional))",
}

TEMPLATE = "<li><strong>{0}</strong>: {1}</li>\n"

if not os.path.exists("languages.json"):
    print("Could not find 'languages.json' in current directory!")
    print("Save the output from https://www.transifex.com/api/2/project/keepassxc/languages")
    exit(0)

with open("languages.json") as json_file:
    output = open("translators.html", "w", encoding="utf-8")
    languages = json.load(json_file)
    for lang in languages:
        code = lang["language_code"]
        if code not in LANGS:
            print("WARNING: Could not find language code:", code)
            continue
        translators = ", ".join(sorted(lang["reviewers"] + lang["translators"], key=str.casefold))
        output.write(TEMPLATE.format(LANGS[code], translators))
    output.close()
    print("Language translators written to 'translators.html'!")