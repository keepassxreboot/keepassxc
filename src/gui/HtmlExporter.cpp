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

#include "HtmlExporter.h"

#include <QBuffer>
#include <QFile>

#include "core/Group.h"
#include "core/Metadata.h"
#include "gui/Icons.h"

namespace
{
    QString PixmapToHTML(const QPixmap& pixmap)
    {
        if (pixmap.isNull()) {
            return "";
        }

        // Based on https://stackoverflow.com/a/6621278
        QByteArray a;
        QBuffer buffer(&a);
        pixmap.save(&buffer, "PNG");
        return QString("<img src=\"data:image/png;base64,") + a.toBase64() + "\"/>";
    }

    QString formatHTML(const QString& value)
    {
        return value.toHtmlEscaped().replace(" ", "&nbsp;").replace('\n', "<br>");
    }

    QString formatAttribute(const QString& key,
                            const QString& value,
                            const QString& classname,
                            const QString& templt = QString("<tr><th>%1</th><td class=\"%2\">%3</td></tr>"))
    {
        const auto& formatted_attribute = templt;
        if (!value.isEmpty()) {
            // Format key as well -> Translations into other languages may have non-standard chars
            return formatted_attribute.arg(formatHTML(key), classname, formatHTML(value));
        }
        return {};
    }

    QString formatAttribute(const Entry& entry,
                            const QString& key,
                            const QString& value,
                            const QString& classname,
                            const QString& templt = QString("<tr><th>%1</th><td class=\"%2\">%3</td></tr>"))
    {
        if (value.isEmpty())
            return {};
        return formatAttribute(key, entry.resolveMultiplePlaceholders(value), classname, templt);
    }

    QString formatEntry(const Entry& entry)
    {
        // Here we collect the table rows with this entry's data fields
        QString item;

        // Output the fixed fields
        item.append(formatAttribute(entry, QObject::tr("User name"), entry.username(), "username"));

        item.append(formatAttribute(entry, QObject::tr("Password"), entry.password(), "password"));

        if (!entry.url().isEmpty()) {
            constexpr auto maxlen = 100;
            QString displayedURL(formatHTML(entry.url()).mid(0, maxlen));

            if (displayedURL.size() == maxlen) {
                displayedURL.append("&hellip;");
            }

            item.append(formatAttribute(entry,
                                        QObject::tr("URL"),
                                        entry.url(),
                                        "url",
                                        R"(<tr><th>%1</th><td class="%2"><a href="%3">%4</a></td></tr>)")
                            .arg(entry.resolveMultiplePlaceholders(displayedURL)));
        }

        item.append(formatAttribute(entry, QObject::tr("Notes"), entry.notes(), "notes"));

        // Now add the attributes (if there are any)
        const auto* const attr = entry.attributes();
        if (attr && !attr->customKeys().isEmpty()) {
            for (const auto& key : attr->customKeys()) {
                item.append(formatAttribute(entry, key, attr->value(key), "attr"));
            }
        }
        return item;
    }
} // namespace

bool HtmlExporter::exportDatabase(const QString& filename, const QSharedPointer<const Database>& db)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_error = file.errorString();
        return false;
    }
    return exportDatabase(&file, db);
}

QString HtmlExporter::errorString() const
{
    return m_error;
}

bool HtmlExporter::exportDatabase(QIODevice* device, const QSharedPointer<const Database>& db)
{
    const auto meta = db->metadata();
    if (!meta) {
        m_error = "Internal error: metadata is NULL";
        return false;
    }

    const auto header = QString("<html>"
                                "<head>"
                                "<meta charset=\"UTF-8\">"
                                "<title>"
                                + meta->name().toHtmlEscaped()
                                + "</title>"
                                  "<style>"
                                  "body "
                                  "{ font-family: \"Open Sans\", Helvetica, Arial, sans-serif; }"
                                  "h3 "
                                  "{ margin-left: 2em; }"
                                  "table "
                                  "{ margin-left: 4em; } "
                                  "th, td "
                                  "{ text-align: left; vertical-align: top; padding: 1px; }"
                                  "th "
                                  "{ min-width: 5em; width: 20%; } "
                                  ".username, .password, .url, .attr "
                                  "{ font-size: larger; font-family: monospace; } "
                                  ".notes "
                                  "{ font-size: medium; } "
                                  "</style>"
                                  "</head>\n"
                                  "<body>"
                                  "<h1>"
                                + meta->name().toHtmlEscaped()
                                + "</h1>"
                                  "<p>"
                                + meta->description().toHtmlEscaped().replace("\n", "<br>")
                                + "</p>"
                                  "<p><code>"
                                + db->filePath().toHtmlEscaped() + "</code></p>");
    const auto footer = QString("</body>"
                                "</html>");

    if (device->write(header.toUtf8()) == -1) {
        m_error = device->errorString();
        return false;
    }

    if (db->rootGroup()) {
        if (!writeGroup(*device, *db->rootGroup())) {
            return false;
        }
    }

    if (device->write(footer.toUtf8()) == -1) {
        m_error = device->errorString();
        return false;
    }

    return true;
}

bool HtmlExporter::writeGroup(QIODevice& device, const Group& group, QString path)
{
    // Don't output the recycle bin
    if (&group == group.database()->metadata()->recycleBin()) {
        return true;
    }

    if (!path.isEmpty()) {
        path.append(" &rarr; ");
    }
    path.append(group.name().toHtmlEscaped());

    // Output the header for this group (but only if there are
    // any notes or  entries in this group, otherwise we'd get
    // a header with nothing after it, which looks stupid)
    const auto& entries = group.entries();
    const auto notes = group.notes();
    if (!entries.empty() || !notes.isEmpty()) {

        // Header line
        auto header = QString("<hr><h2>");
        header.append(PixmapToHTML(Icons::groupIconPixmap(&group, IconSize::Medium)));
        header.append("&nbsp;");
        header.append(path);
        header.append("</h2>\n");

        // Group notes
        if (!notes.isEmpty()) {
            header.append("<p>");
            header.append(notes.toHtmlEscaped().replace("\n", "<br>"));
            header.append("</p>");
        }

        // Output it
        if (device.write(header.toUtf8()) == -1) {
            m_error = device.errorString();
            return false;
        }
    }

    // Begin the table for the entries in this group
    auto table = QString("<table width=\"100%\">");

    // Output the entries in this group
    for (const auto entry : entries) {
        auto formatted_entry = formatEntry(*entry);

        if (formatted_entry.isEmpty())
            continue;

        // Output it into our table. First the left side with
        // icon and entry title ...
        table += "<tr>";
        table += "<td width=\"1%\">" + PixmapToHTML(Icons::entryIconPixmap(entry, IconSize::Medium)) + "</td>";
        table += "<td width=\"19%\" valign=\"top\"><h3>" + entry->title().toHtmlEscaped() + "</h3></td>";

        // ... then the right side with the data fields
        table += "<td style=\"padding-bottom: 0.5em;\"><table width=\"100%\">" + formatted_entry + "</table></td>";
        table += "</tr>";
    }

    // Output the complete table of this group
    table.append("</table>\n");
    if (device.write(table.toUtf8()) == -1) {
        m_error = device.errorString();
        return false;
    }

    // Recursively output the child groups
    const auto& children = group.children();
    for (const auto child : children) {
        if (child && !writeGroup(device, *child, path)) {
            return false;
        }
    }

    return true;
}
