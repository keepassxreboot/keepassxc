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

#include "QrTotpWidget.h"

#include "core/Totp.h"
#include "qrdecoder.h"

#include <ZXing/ReadBarcode.h>
#include <ZXing/ReaderOptions.h>

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>

namespace QrDecoder
{

QrTotpWidget::QrTotpWidget(QWidget* parent)
    : QWidget(parent)
    , m_uriEdit(new QLineEdit(this))
{
    m_uriEdit->installEventFilter(this);

    m_uriEdit->setPlaceholderText(QStringLiteral("otpauth://totp/..."));

    auto pasteButton = new QPushButton(tr("Paste QR code"), this);
    auto applyButton = new QPushButton(tr("Apply"), this);

    pasteButton->installEventFilter(this);
    applyButton->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_uriEdit);
    layout->addWidget(pasteButton);
    layout->addWidget(applyButton);
    layout->addStretch();

    connect(pasteButton, &QPushButton::clicked, this, &QrTotpWidget::pasteImage);

    connect(applyButton, &QPushButton::clicked, this, [this] {
        const auto uri = m_uriEdit->text().trimmed();

        if (uri.isEmpty()) {
            return;
        }

        const auto settings = Totp::parseSettings(uri, {});

        if (!settings) {
            return;
        }

        emit settingsReady(settings);
    });
}

bool QrTotpWidget::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);

    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->matches(QKeySequence::Paste)) {
            pasteClipboard();
            keyEvent->accept();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void QrTotpWidget::pasteClipboard()
{
    const auto* clipboard = QApplication::clipboard();
    if (!clipboard) {
        return;
    }

    const auto* mimeData = clipboard->mimeData();
    if (!mimeData) {
        return;
    }

    // First try plain text. This allows directly pasting:
    //
    // otpauth://totp/keepassxc:test%40lab.com?secret=...&issuer=keepassxc&period=60
    //
    if (mimeData->hasText()) {
        const auto text = mimeData->text().trimmed();

        if (text.startsWith(QStringLiteral("otpauth://"), Qt::CaseInsensitive)) {
            const auto settings = Totp::parseSettings(text, {});

            if (settings) {
                m_uriEdit->setText(text);
                emit settingsReady(settings);
                return;
            }
        }
    }

    // Otherwise try an image containing a QR code.
    if (mimeData->hasImage()) {
        const auto image = qvariant_cast<QImage>(mimeData->imageData());

        if (!image.isNull()) {
            decodeImage(image);
        }
    }
}

void QrTotpWidget::pasteImage()
{
    pasteClipboard();
}

void QrTotpWidget::decodeImage(const QImage& image)
{
    if (image.isNull()) {
        return;
    }

    const auto converted = image.convertToFormat(QImage::Format_RGBA8888);

    ZXing::ImageView imageView(converted.constBits(),
                               converted.width(),
                               converted.height(),
                               ZXing::ImageFormat::RGBA,
                               converted.bytesPerLine());

    ZXing::ReaderOptions options;
    options.setFormats(ZXing::BarcodeFormat::QRCode);

    const auto barcodes = ZXing::ReadBarcodes(imageView, options);

    for (const auto& barcode : barcodes) {
        const auto text = QString::fromStdString(barcode.text()).trimmed();

        if (!text.startsWith(QStringLiteral("otpauth://"), Qt::CaseInsensitive)) {
            continue;
        }

        const auto settings = Totp::parseSettings(text, {});

        if (!settings) {
            continue;
        }

        m_uriEdit->setText(text);
        emit settingsReady(settings);
        return;
    }
}

} // namespace QrDecoder