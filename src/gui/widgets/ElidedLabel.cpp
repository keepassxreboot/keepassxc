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

#include "ElidedLabel.h"

namespace
{
    const QString htmlLinkTemplate("<a href=\"%1\">%2</a>");
}

ElidedLabel::ElidedLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f)
    , m_elideMode(Qt::ElideMiddle)
{
    connect(this, SIGNAL(elideModeChanged(Qt::TextElideMode)), this, SLOT(updateElidedText()));
    connect(this, SIGNAL(rawTextChanged(QString)), this, SLOT(updateElidedText()));
    connect(this, SIGNAL(urlChanged(QString)), this, SLOT(updateElidedText()));
}

ElidedLabel::ElidedLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
    : ElidedLabel(parent, f)
{
    setText(text);
}

Qt::TextElideMode ElidedLabel::elideMode() const
{
    return m_elideMode;
}

QString ElidedLabel::rawText() const
{
    return m_rawText;
}

QString ElidedLabel::url() const
{
    return m_url;
}

void ElidedLabel::setElideMode(Qt::TextElideMode elideMode)
{
    if (m_elideMode == elideMode) {
        return;
    }

    if (m_elideMode != Qt::ElideNone) {
        setWordWrap(false);
    }

    m_elideMode = elideMode;
    emit elideModeChanged(m_elideMode);
}

void ElidedLabel::setRawText(const QString& elidedText)
{
    if (m_rawText == elidedText) {
        return;
    }

    m_rawText = elidedText;
    emit rawTextChanged(m_rawText);
}

void ElidedLabel::setUrl(const QString& url)
{
    if (m_url == url) {
        return;
    }

    m_url = url;
    emit urlChanged(m_url);
}

void ElidedLabel::clear()
{
    setRawText(QString());
    setElideMode(Qt::ElideMiddle);
    setUrl(QString());
    QLabel::clear();
}

void ElidedLabel::updateElidedText()
{
    if (m_rawText.isEmpty()) {
        QLabel::clear();
        return;
    }

    QString displayText = m_rawText;
    if (m_elideMode != Qt::ElideNone) {
        const QFontMetrics metrix(font());
        displayText = metrix.elidedText(m_rawText, m_elideMode, width() - 2);
    }

    bool hasUrl = !m_url.isEmpty();
    setText(hasUrl ? htmlLinkTemplate.arg(m_url.toHtmlEscaped(), displayText) : displayText);
    setOpenExternalLinks(!hasUrl);
}

void ElidedLabel::resizeEvent(QResizeEvent* event)
{
    updateElidedText();
    QLabel::resizeEvent(event);
}
