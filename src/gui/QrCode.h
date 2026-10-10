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

#ifndef KEEPASSXC_QRCODE_H
#define KEEPASSXC_QRCODE_H

#include <QScopedPointer>

class QIODevice;
class QString;

struct QrCodePrivate;

class QrCode
{

public:
    enum class ErrorCorrectionLevel : int
    {
        LOW = 0,
        MEDIUM,
        QUARTILE,
        HIGH
    };

    explicit QrCode(const QString& data, const ErrorCorrectionLevel ecl = ErrorCorrectionLevel::HIGH);

    QrCode();
    ~QrCode();

    bool isValid() const;
    void writeSvg(QIODevice* outputDevice, const int dpi, const int margin = 4) const;

private:
    QScopedPointer<QrCodePrivate> d_ptr;
};

#endif // KEEPASSXC_QRCODE_H
