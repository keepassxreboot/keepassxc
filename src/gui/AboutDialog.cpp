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
#include "git-info.h"

#include <QClipboard>
#include <QSysInfo>

static const QString aboutMaintainers = R"(
<p><ul>
    <li>Jonathan White (<a href="https://github.com/droidmonkey">droidmonkey</a>)</li>
    <li>Janek Bevendorff (<a href="https://github.com/phoerious">phoerious</a>)</li>
    <li><a href="https://github.com/TheZ3ro">TheZ3ro</a></li>
    <li>Louis-Bertrand (<a href="https://github.com/louib">louib</a>)</li>
    <li>Weslly Honorato (<a href="https://github.com/weslly">weslly</a>)</li>
    <li>Toni Spets (<a href="https://github.com/hifi">hifi</a>)</li>
    <li>Sami V&auml;nttinen (<a href="https://github.com/varjolintu">varjolintu</a>)</li>
</ul></p>
)";

static const QString aboutContributors = R"(
<h3>VIP Patreon Supporters:</h3>
<ul>
    <li>John Cook</li>
    <li>Max Anderson</li>
    <li>l0b0</li>
    <li>NarwhalOfAges</li>
    <li>Caleb Currie</li>
    <li>Igor Zinovik</li>
    <li>Morgan Courbet</li>
    <li>Sergiu Coroi</li>
</ul>
<h3>Notable Code Contributions:</h3>
<ul>
    <li>droidmonkey</li>
    <li>phoerious</li>
    <li>TheZ3ro</li>
    <li>louib</li>
    <li>weslly</li>
    <li>varjolintu (KeePassXC-Browser)</li>
    <li>hifi (SSH Agent)</li>
    <li>ckieschnick (KeeShare)</li>
    <li>seatedscribe (CSV Import)</li>
    <li>brainplot (many improvements)</li>
    <li>kneitinger (many improvements)</li>
    <li>frostasm (many improvements)</li>
    <li>fonic (Entry Table View)</li>
    <li>kylemanna (YubiKey)</li>
    <li>keithbennett (KeePassHTTP)</li>
    <li>Typz (KeePassHTTP)</li>
    <li>denk-mal (KeePassHTTP)</li>
    <li>angelsl (KDBX 4)</li>
    <li>debfx (KeePassX)</li>
    <li>BlueIce (KeePassX)</li>
</ul>
<h3>Patreon Supporters:</h3>
<ul>
    <li>Ashura</li>
    <li>Alexanderjb</li>
    <li>Andreas Kollmann</li>
    <li>Richard Ames</li>
    <li>Christian Rasmussen</li>
    <li>Gregory Werbin</li>
    <li>Nuutti Toivola</li>
    <li>SLmanDR</li>
    <li>Tyler Gass</li>
    <li>Lionel Laské</li>
    <li>Dmitrii Galinskii</li>
    <li>Sergei Maximov</li>
    <li>John-Ivar</li>
    <li>Clayton Casciato</li>
</ul>
<h3>Translations:</h3>
<ul>
    <li><strong>Basque</strong>: azken_tximinoa, Hey_neken</li>
    <li><strong>Catalan</strong>: capitantrueno, dsoms, mcus, raulua, ZJaume</li>
    <li><strong>Chinese (China)</strong>: Biggulu, Brandon_c, hoilc, ligyxy,
        vc5, Small_Ku</li>
    <li><strong>Chinese (Taiwan)</strong>: BestSteve, MiauLightouch, Small_Ku,
        yan12125, ymhuang0808</li>
    <li><strong>Czech</strong>: DanielMilde, JosefVitu, pavelb, tpavelek</li>
    <li><strong>Danish</strong>: nlkl</li>
    <li><strong>Dutch</strong>: apie, bartlibert, evanoosten, fvw, KnooL, srgvg,
        Vistaus, wanderingidea</li>
    <li><strong>Finnish</strong>: artnay, Jarppi, MawKKe</li>
    <li><strong>French</strong>: A1RO, aghilas.messara, bisaloo, frgnca,
        ggtr1138, gilbsgilbs, gtalbot, Gui13, iannick, jlutran, kyodev, logut,
        MartialBis, narzb, pBouillon, plunkets, Raphi111, Scrat15, tl_pierre,
        wilfriedroset</li>
    <li><strong>German</strong>: antsas, BasicBaer, Calyrx, codejunky,
        DavidHamburg, eth0, for1real, jensrutschmann, joe776, kflesch,
        MarcEdinger, marcbone, mcliquid, mfernau77, montilo, nursoda, omnisome4,
        origin_de, pcrcoding, phoerious, rgloor, transi_222, vlenzer, waster</li>
    <li><strong>Greek</strong>: magkopian, nplatis, tassos.b, xinomilo</li>
    <li><strong>Hungarian</strong>: bubu, meskobalazs, urbalazs</li>
    <li><strong>Indonesian</strong>: zk</li>
    <li><strong>Italian</strong>: amaxis, bovirus, duncanmid, FranzMari, lucaim,
        Mte90, Peo, TheZ3ro, tosky, VosaxAlo</li>
    <li><strong>Japanese</strong>: masoo, metalic_cat, p2635,
        Shinichirou_Yamada, vargas.peniel, vmemjp, yukinakato</li>
    <li><strong>Korean</strong>: cancantun, peremen</li>
    <li><strong>Lithuanian</strong>: Moo</li>
    <li><strong>Polish</strong>: keypress, konradmb, mrerexx, psobczak</li>
    <li><strong>Portuguese (Brazil)</strong>: danielbibit, fabiom, flaviobn,
        vitor895, weslly</li>
</ul>
)";

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

    m_ui->maintainers->setText(aboutMaintainers);
    m_ui->contributors->setText(aboutContributors);

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
