/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "ClipboardMime.h"

#include <QMimeData>

#ifdef Q_OS_MACOS
#include "gui/osutils/macutils/MacPasteboard.h"

#include <QPointer>
#endif

namespace
{
#if defined(Q_OS_MACOS)
    QPointer<MacPasteboard> s_pasteboard(nullptr);

    void initializeNativeConverters()
    {
        // This object lives for the whole program lifetime and we cannot delete it on exit,
        // so ignore leak warnings. See https://bugreports.qt.io/browse/QTBUG-54832
        if (!s_pasteboard) {
            s_pasteboard = new MacPasteboard();
        }
    }
#else
    void initializeNativeConverters()
    {
    }
#endif
} // namespace

namespace ClipboardMime
{
    QMimeData* createSecretMimeData(const QString& text)
    {
        initializeNativeConverters();

        auto* mime = new QMimeData;
        mime->setText(text);
#if defined(Q_OS_MACOS)
        mime->setData("application/x-nspasteboard-concealed-type", text.toUtf8());
#elif defined(Q_OS_UNIX)
        mime->setData("x-kde-passwordManagerHint", QByteArrayLiteral("secret"));
#elif defined(Q_OS_WIN)
        mime->setData("ExcludeClipboardContentFromMonitorProcessing", QByteArrayLiteral("1"));
        mime->setData("CanIncludeInClipboardHistory", {4, '\0'});
        mime->setData("CanUploadToCloudClipboard", {4, '\0'});
#endif
        return mime;
    }

    QStringList secretFormats()
    {
#if defined(Q_OS_MACOS)
        return {QStringLiteral("application/x-nspasteboard-concealed-type")};
#elif defined(Q_OS_UNIX)
        return {QStringLiteral("x-kde-passwordManagerHint")};
#elif defined(Q_OS_WIN)
        return {QStringLiteral("ExcludeClipboardContentFromMonitorProcessing"),
                QStringLiteral("CanIncludeInClipboardHistory"),
                QStringLiteral("CanUploadToCloudClipboard")};
#else
        return {};
#endif
    }
} // namespace ClipboardMime
