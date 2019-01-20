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
#include "crypto/Crypto.h"
#include "version.h"

#include <QClipboard>
#include <QSysInfo>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::AboutDialog())
{
    m_ui->setupUi(this);

    resize(minimumSize());
    setWindowFlags(Qt::Sheet);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->nameLabel->setText(m_ui->nameLabel->text().replace("${VERSION}", KEEPASSXC_VERSION));
    QFont nameLabelFont = m_ui->nameLabel->font();
    nameLabelFont.setPointSize(nameLabelFont.pointSize() + 4);
    m_ui->nameLabel->setFont(nameLabelFont);

    m_ui->iconLabel->setPixmap(filePath()->applicationIcon().pixmap(48));

    QString commitHash;
    if (!QString(GIT_HEAD).isEmpty()) {
        commitHash = GIT_HEAD;
    } else if (!QString(DIST_HASH).contains("Format")) {
        commitHash = DIST_HASH;
    }

    QString debugInfo = "KeePassXC - ";
    debugInfo.append(tr("Version %1").arg(KEEPASSXC_VERSION).append("\n"));
#ifndef KEEPASSXC_BUILD_TYPE_RELEASE
    debugInfo.append(tr("Build Type: %1").arg(KEEPASSXC_BUILD_TYPE).append("\n"));
#endif
    if (!commitHash.isEmpty()) {
        debugInfo.append(tr("Revision: %1").arg(commitHash.left(7)).append("\n"));
    }

#ifdef KEEPASSXC_DIST
    debugInfo.append(tr("Distribution: %1").arg(KEEPASSXC_DIST_TYPE).append("\n"));
#endif

    debugInfo.append("\n").append(
        QString("%1\n- Qt %2\n- %3\n\n")
            .arg(tr("Libraries:"), QString::fromLocal8Bit(qVersion()), Crypto::backendVersion()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    debugInfo.append(tr("Operating system: %1\nCPU architecture: %2\nKernel: %3 %4")
                         .arg(QSysInfo::prettyProductName(),
                              QSysInfo::currentCpuArchitecture(),
                              QSysInfo::kernelType(),
                              QSysInfo::kernelVersion()));

    debugInfo.append("\n\n");
#endif

    QString extensions;
#ifdef WITH_XC_AUTOTYPE
    extensions += "\n- " + tr("Auto-Type");
#endif
#ifdef WITH_XC_BROWSER
    extensions += "\n- " + tr("Browser Integration");
#endif
#ifdef WITH_XC_SSHAGENT
    extensions += "\n- " + tr("SSH Agent");
#endif
#if defined(WITH_XC_KEESHARE_SECURE) && defined(WITH_XC_KEESHARE_INSECURE)
    extensions += "\n- " + tr("KeeShare (signed and unsigned sharing)");
#elif defined(WITH_XC_KEESHARE_SECURE)
    extensions += "\n- " + tr("KeeShare (only signed sharing)");
#elif defined(WITH_XC_KEESHARE_INSECURE)
    extensions += "\n- " + tr("KeeShare (only unsigned sharing)");
#endif
#ifdef WITH_XC_YUBIKEY
    extensions += "\n- " + tr("YubiKey");
#endif
#ifdef WITH_XC_TOUCHID
    extensions += "\n- " + tr("TouchID");
#endif

    if (extensions.isEmpty())
        extensions = " " + tr("None");

    debugInfo.append(tr("Enabled extensions:").append(extensions));

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
