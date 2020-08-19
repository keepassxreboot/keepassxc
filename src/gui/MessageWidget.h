/*
 *  Copyright (C) 2015 Pedro Alves <devel@pgalves.com>
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

#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include "gui/KMessageWidget.h"

class QTimer;

class MessageWidget : public KMessageWidget
{
    Q_OBJECT

public:
    explicit MessageWidget(QWidget* parent = nullptr);

    int autoHideTimeout() const;

    static const int DefaultAutoHideTimeout;
    static const int LongAutoHideTimeout;
    static const int DisableAutoHide;

    void setAnimate(bool state);

signals:
    void showAnimationStarted();
    void hideAnimationStarted();

public slots:
    void showMessage(const QString& text, MessageWidget::MessageType type);
    void showMessage(const QString& text, MessageWidget::MessageType type, int autoHideTimeout);
    void hideMessage();
    void setAutoHideTimeout(int autoHideTimeout);
    static void openHttpUrl(QString const& url);

private:
    QTimer* m_autoHideTimer;
    int m_autoHideTimeout;
    bool m_animate = true;
};

#endif // MESSAGEWIDGET_H
