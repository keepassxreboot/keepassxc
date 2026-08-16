#include "QrDecoder.h"

#include <QImage>
#include <QString>

#include <ZXing/BarcodeFormat.h>
#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ReaderOptions.h>

namespace QrDecoder
{
QString QrDecoder::decode(const QImage& input)
{
    if (input.isNull()) {
        return {};
    }

    const auto image = input.convertToFormat(QImage::Format_Grayscale8);

    const auto imageView = ZXing::ImageView(
        image.constBits(),
        image.width(),
        image.height(),
        ZXing::ImageFormat::Lum,
        static_cast<int>(image.bytesPerLine()));

    ZXing::ReaderOptions options;
    options.setFormats(ZXing::BarcodeFormat::QRCode);

    const auto barcodes = ZXing::ReadBarcodes(imageView, options);

    if (barcodes.empty()) {
        return {};
    }

    return QString::fromStdString(barcodes.front().text());
}
} // namespace QrDecoder
