/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
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

#include "Translator.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegExp>
#include <QTranslator>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/FilePath.h"

void Translator::installTranslator()
{
    QString language = config()->get("GUI/Language").toString();
    if (language == "system" || language.isEmpty()) {
        language = QLocale::system().name();
    }

    if (!installTranslator(language)) {
        // English fallback still needs translations for plurals
        if (!installTranslator("en_plurals")) {
            qWarning("Couldn't load translations.");
        }
    }

    installQtTranslator(language);

    availableLanguages();
}

QList<QPair<QString, QString> > Translator::availableLanguages()
{
    QStringList paths;
#ifdef QT_DEBUG
    paths.append(QString("%1/share/translations").arg(KEEPASSX_BINARY_DIR));
#endif
    paths.append(filePath()->dataPath("translations"));

    QList<QPair<QString, QString> > languages;
    languages.append(QPair<QString, QString>("system", "System default"));

    QRegExp regExp("keepassx_([a-zA-Z_]+)\\.qm", Qt::CaseInsensitive, QRegExp::RegExp2);
    Q_FOREACH (const QString& path, paths) {
        Q_FOREACH (const QString& filename, QDir(path).entryList()) {
            if (regExp.exactMatch(filename)) {
                QString langcode = regExp.cap(1);
                if (langcode == "en_plurals") {
                    langcode = "en";
                }

                languages.append(QPair<QString, QString>(langcode,
                                                         QLocale::languageToString(QLocale(langcode).language())));
            }
        }
    }

    return languages;
}

bool Translator::installTranslator(const QString& language)
{
    QStringList paths;
#ifdef QT_DEBUG
    paths.append(QString("%1/share/translations").arg(KEEPASSX_BINARY_DIR));
#endif
    paths.append(filePath()->dataPath("translations"));

    Q_FOREACH (const QString& path, paths) {
        if (installTranslator(language, path)) {
            return true;
        }
    }

    return false;
}

bool Translator::installTranslator(const QString& language, const QString& path)
{
    QTranslator* translator = new QTranslator(qApp);
    if (translator->load(QString("keepassx_").append(language), path)) {
        QCoreApplication::installTranslator(translator);
        return true;
    }
    else {
        delete translator;
        return false;
    }
}

bool Translator::installQtTranslator(const QString& language)
{
    QTranslator* qtTranslator = new QTranslator(qApp);
    if (qtTranslator->load(QString("%1/qt_%2").arg(QLibraryInfo::location(QLibraryInfo::TranslationsPath), language))) {
        QCoreApplication::installTranslator(qtTranslator);
        return true;
    }
    else {
        delete qtTranslator;
        return false;
    }
}
