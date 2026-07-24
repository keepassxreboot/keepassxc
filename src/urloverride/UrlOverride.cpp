/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "UrlOverride.h"

#include "core/Config.h"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#ifdef Q_OS_WIN
#include <Windows.h>

namespace
{
    // Reads the PE header of an executable to determine whether it is a console-subsystem
    // application (e.g. ssh.exe) as opposed to a GUI-subsystem one (e.g. firefox.exe). Only
    // console-subsystem programs need a console window allocated for them; forcing one for a
    // GUI program would just pop up an empty, unwanted console window next to it.
    bool isConsoleSubsystemExecutable(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        IMAGE_DOS_HEADER dosHeader;
        if (file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader)) != sizeof(dosHeader)
            || dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            return false;
        }

        if (!file.seek(dosHeader.e_lfanew)) {
            return false;
        }

        DWORD peSignature;
        if (file.read(reinterpret_cast<char*>(&peSignature), sizeof(peSignature)) != sizeof(peSignature)
            || peSignature != IMAGE_NT_SIGNATURE) {
            return false;
        }

        IMAGE_FILE_HEADER fileHeader;
        if (file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader)) != sizeof(fileHeader)) {
            return false;
        }

        const qint64 optionalHeaderStart = file.pos();
        WORD magic;
        if (file.read(reinterpret_cast<char*>(&magic), sizeof(magic)) != sizeof(magic)
            || !file.seek(optionalHeaderStart)) {
            return false;
        }

        WORD subsystem;
        if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            IMAGE_OPTIONAL_HEADER64 optionalHeader;
            if (file.read(reinterpret_cast<char*>(&optionalHeader), sizeof(optionalHeader)) != sizeof(optionalHeader)) {
                return false;
            }
            subsystem = optionalHeader.Subsystem;
        } else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            IMAGE_OPTIONAL_HEADER32 optionalHeader;
            if (file.read(reinterpret_cast<char*>(&optionalHeader), sizeof(optionalHeader)) != sizeof(optionalHeader)) {
                return false;
            }
            subsystem = optionalHeader.Subsystem;
        } else {
            return false;
        }

        return subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI;
    }
} // namespace
#endif

namespace
{
    // Rules are stored as an XML string under a single Config key, the same way KeeShare stores
    // its (also structured, list-shaped) settings via KeeShareSettings::serialize/deserialize.
    QString serializeRules(const QList<UrlOverride::Rule>& rules)
    {
        QString buffer;
        QXmlStreamWriter writer(&buffer);
        writer.writeStartDocument();
        writer.writeStartElement("UrlOverrides");
        for (const auto& rule : rules) {
            writer.writeStartElement("Rule");
            writer.writeAttribute("Enabled", rule.enabled ? "1" : "0");
            writer.writeTextElement("Scheme", rule.scheme);
            writer.writeTextElement("Command", rule.command);
            writer.writeEndElement();
        }
        writer.writeEndElement();
        writer.writeEndDocument();
        return buffer;
    }

    QList<UrlOverride::Rule> deserializeRules(const QString& raw)
    {
        QList<UrlOverride::Rule> rules;
        QXmlStreamReader reader(raw);
        if (!reader.readNextStartElement() || reader.name().toString() != QLatin1String("UrlOverrides")) {
            return rules;
        }

        while (reader.readNextStartElement()) {
            if (reader.name().toString() != QLatin1String("Rule")) {
                reader.skipCurrentElement();
                continue;
            }

            UrlOverride::Rule rule;
            rule.enabled = reader.attributes().value("Enabled").toString() != QLatin1String("0");
            while (reader.readNextStartElement()) {
                if (reader.name().toString() == QLatin1String("Scheme")) {
                    rule.scheme = reader.readElementText();
                } else if (reader.name().toString() == QLatin1String("Command")) {
                    rule.command = reader.readElementText();
                } else {
                    reader.skipCurrentElement();
                }
            }
            rules.append(rule);
        }
        return rules;
    }
} // namespace

namespace UrlOverride
{
    QList<Rule> getRules()
    {
        const auto raw = config()->get(Config::UrlOverride_Rules).toString();
        if (raw.isEmpty()) {
            // Nothing has ever been saved for this feature (fresh install, or a fresh config
            // file): seed a disabled example rule so the feature and its placeholder syntax are
            // discoverable in the settings page without doing anything until explicitly enabled.
            // Once the user saves anything (including an empty list), this is never shown again.
            return {{false, "ssh", "cmd://ssh {USERNAME}@{URL:HOST}"}};
        }
        return deserializeRules(raw);
    }

    void setRules(const QList<Rule>& rules)
    {
        QList<Rule> normalizedRules;
        normalizedRules.reserve(rules.size());
        for (const auto& rule : rules) {
            normalizedRules.append({rule.enabled, normalizeScheme(rule.scheme), rule.command});
        }
        config()->set(Config::UrlOverride_Rules, serializeRules(normalizedRules));
    }

    QString findCommand(const QString& url)
    {
        const QString urlScheme = QUrl(url).scheme();
        if (urlScheme.isEmpty()) {
            return {};
        }

        for (const auto& rule : getRules()) {
            // A rule with an empty command has nothing to run: skip it and keep looking, rather
            // than matching and returning an empty command that blocks any lower-priority rule
            // for the same scheme.
            if (!rule.enabled || rule.scheme.isEmpty() || rule.command.isEmpty()) {
                continue;
            }
            if (rule.scheme.compare(urlScheme, Qt::CaseInsensitive) == 0) {
                return rule.command;
            }
        }
        return {};
    }

    QString normalizeScheme(const QString& scheme)
    {
        QString normalized = scheme.trimmed();
        while (normalized.endsWith(QLatin1Char(':')) || normalized.endsWith(QLatin1Char('/'))) {
            normalized.chop(1);
        }
        return normalized;
    }

    void executeCommand(const QString& program, const QStringList& arguments)
    {
#ifdef Q_OS_WIN
        // QProcess::startDetached() does not request a new console for the child process.
        // KeePassXC itself has no console, so without this, a console-subsystem command (e.g.
        // ssh) ends up with no visible window and no way to answer interactive prompts. Only do
        // this for console-subsystem programs; forcing it for a GUI program (e.g. a browser)
        // would just pop up an empty console window alongside it.
        const auto resolvedProgram = QStandardPaths::findExecutable(program);
        if (!resolvedProgram.isEmpty() && isConsoleSubsystemExecutable(resolvedProgram)) {
            QProcess process;
            process.setProgram(program);
            process.setArguments(arguments);
            process.setCreateProcessArgumentsModifier(
                [](QProcess::CreateProcessArguments* args) { args->flags |= CREATE_NEW_CONSOLE; });
            process.startDetached();
            return;
        }
#endif
        QProcess::startDetached(program, arguments);
    }
} // namespace UrlOverride
