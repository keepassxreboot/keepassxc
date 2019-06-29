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

#include "Font.h"

#include <QFontDatabase>

QFont Font::defaultFont()
{
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont);
}

QFont Font::fixedFont()
{
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

#ifdef Q_OS_WIN
    // try to use Consolas on Windows, because the default Courier New has too many similar characters
    QFont consolasFont = QFontDatabase().font("Consolas", fixedFont.styleName(), fixedFont.pointSize());
    const QFont defaultFont;
    if (fixedFont != defaultFont) {
        fixedFont = consolasFont;
    }
#endif

    return fixedFont;
}
