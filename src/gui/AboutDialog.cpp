/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "config-keepassx.h"
#include "core/FilePath.h"
#include "core/Tools.h"

#include <QClipboard>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent),
      m_ui(new Ui::AboutDialog())
{
    m_ui->setupUi(this);

    resize(minimumSize());
    setWindowFlags(Qt::Sheet);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->nameLabel->setText(m_ui->nameLabel->text().replace("${VERSION}", KEEPASSX_VERSION));
    QFont nameLabelFont = m_ui->nameLabel->font();
    nameLabelFont.setPointSize(nameLabelFont.pointSize() + 4);
    m_ui->nameLabel->setFont(nameLabelFont);

    m_ui->iconLabel->setPixmap(filePath()->applicationIcon().pixmap(48));

    QString debugInfo = Tools::getDebugInfo();
    m_ui->debugInfo->setPlainText(debugInfo);

    setAttribute(Qt::WA_DeleteOnClose);
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->copyToClipboard, SIGNAL(clicked()), SLOT(copyToClipboard()));
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::copyToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(m_ui->debugInfo->toPlainText());
}
