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

#ifndef KEEPASSXC_QRTOTPWIDGET_H
#define KEEPASSXC_QRTOTPWIDGET_H

#include <QSharedPointer>
#include <QWidget>

class QImage;
class QLineEdit;

namespace Totp
{
    struct Settings;
}

namespace QrDecoder
{

    class QrTotpWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit QrTotpWidget(QWidget* parent = nullptr);

    signals:
        void settingsReady(const QSharedPointer<Totp::Settings>& settings);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        void pasteClipboard();
        void pasteImage();
        void decodeImage(const QImage& image);

        QLineEdit* m_uriEdit = nullptr;
    };

} // namespace QrDecoder

#endif // KEEPASSXC_QRTOTPWIDGET_H
