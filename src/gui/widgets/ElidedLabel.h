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

#ifndef KEEPASSX_ELIDEDLABEL_H
#define KEEPASSX_ELIDEDLABEL_H

#include <QLabel>

class QResizeEvent;

class ElidedLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode NOTIFY elideModeChanged)
    Q_PROPERTY(QString rawText READ rawText WRITE setRawText NOTIFY rawTextChanged)
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
public:
    explicit ElidedLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    Qt::TextElideMode elideMode() const;
    QString rawText() const;
    QString url() const;

public slots:
    void setElideMode(Qt::TextElideMode elideMode);
    void setRawText(const QString& rawText);
    void setUrl(const QString& url);
    void clear();

signals:
    void elideModeChanged(Qt::TextElideMode elideMode);
    void rawTextChanged(QString rawText);
    void urlChanged(QString url);

private slots:
    void updateElidedText();

private:
    void resizeEvent(QResizeEvent* event) override;

    Qt::TextElideMode m_elideMode;
    QString m_rawText;
    QString m_url;
};

#endif // KEEPASSX_ELIDEDLABEL_H
