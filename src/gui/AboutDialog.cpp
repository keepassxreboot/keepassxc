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
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "gui/Icons.h"

#include <QClipboard>

static const QString aboutMaintainers = R"(
<p><ul>
    <li>Jonathan White (<a href="https://github.com/droidmonkey">droidmonkey</a>)</li>
    <li>Janek Bevendorff (<a href="https://github.com/phoerious">phoerious</a>)</li>
    <li>Sami V&auml;nttinen (<a href="https://github.com/varjolintu">varjolintu</a>)</li>
    <li>Toni Spets (<a href="https://github.com/hifi">hifi</a>)</li>
    <li>Louis-Bertrand (<a href="https://github.com/louib">louib</a>)</li>
    <li><a href="https://github.com/TheZ3ro">TheZ3ro</a> (retired)</li>
</ul></p>
)";

static const QString aboutContributors = R"(
<style>ul { list-style: disk; margin-left: 12px; -qt-list-indent: 0; }</style>
<h3>VIP Patreon Supporters:</h3>
<ul>
    <li>Sergey Vilgelm</li>
    <li>Victor Engmark</li>
    <li>NarwhalOfAges</li>
    <li>No Name</li>
    <li>SG</li>
    <li>Riley Moses</li>
    <li>Esteban Martinez</li>
    <li>Marc Morocutti</li>
    <li>Zivix</li>
    <li>Benedikt Heine</li>
    <li>Hugo Locurcio</li>
    <li>William Petrides</li>
    <li>Gunar Gessner</li>
    <li>Christian Wittenhorst</li>
    <li>Matt Cardarelli</li>
    <li>Paul Ammann</li>
    <li>Steve Isom</li>
    <li>GodSpell</li>
    <li>Lionel Laské</li>
    <li>Daniel Epp</li>
    <li>Oleksii Aleksieiev</li>
    <li>Julian Stier</li>
    <li>Ruben Schade</li>
    <li>Bernhard</li>
    <li>Wojciech Kozlowski</li>
    <li>Caleb Currie</li>
    <li>Morgan Courbet</li>
    <li>Kyle Kneitinger</li>
    <li>Chris Sohns</li>
    <li>Shmavon Gazanchyan</li>
    <li>xjdwc</li>
    <li>Igor Zinovik</li>
    <li>Jeff</li>
    <li>Max Andersen</li>
    <li>super scampy</li>
    <li>Mischa Peters</li>
    <li>Rainer-Maria Fritsch</li>
    <li>Micha Ober</li>
    <li>Ivan Gromov</li>
    <li>Joshua Go</li>
    <li>pancakeplant</li>
    <li>Hans-Joachim Forker</li>
    <li>Nicolas Vandemaele</li>
    <li>Saturnio</li>
    <li>Robert Schaffar-Taurok</li>
    <li>Mike</li>
    <li>Thomas Renz</li>
    <li>Toby Cline</li>
    <li>Emre Dessoi</li>
    <li>Michael Babnick</li>
    <li>kernellinux</li>
    <li>Patrick Evans</li>
    <li>Marco</li>
    <li>Jeremy Rubin</li>
    <li>Korbi</li>
    <li>andreas</li>
    <li>Tyche's tidings</li>
    <li>Daniel Kuebler</li>
    <li>Brandon Corujo</li>
    <li>AheroX</li>
    <li>Alexandre G</li>
    <li>AshinaGa</li>
    <li>BYTEBOLT</li>
    <li>CEH</li>
    <li>Chris Stone</li>
    <li>Christof Böckler</li>
    <li>Claude</li>
    <li>CzLer</li>
    <li>Daniel Burridge</li>
    <li>dark</li>
    <li>Dave G</li>
    <li>David Bowers</li>
    <li>dickv</li>
    <li>fp4</li>
    <li>Huser IT Solutions GmbH</li>
    <li>irf</li>
    <li>Isaiah Rahmany</li>
    <li>JackNYC</li>
    <li>Jacob Emmert-Aronson</li>
    <li>John Donadeo</li>
    <li>Kosta Medinsky</li>
    <li>leinad987</li>
    <li>Lux</li>
    <li>marek</li>
    <li>mattlongname</li>
    <li>mattock</li>
    <li>Max Christian Pohle</li>
    <li>nta/norma</li>
    <li>picatsv</li>
    <li>proto</li>
    <li>Raymond Lau</li>
    <li>Waido</li>
    <li>Weinmann Willy</li>
    <li>WildMage</li>
</ul>
<h3>VIP GitHub Sponsors:</h3>
<ul>
    <li>mercedes-benz</li>
    <li>tiangolo</li>
    <li>mrniko</li>
    <li>rszamszur</li>
</ul>
<h3>Notable Code Contributions:</h3>
<ul>
    <li>droidmonkey</li>
    <li>phoerious</li>
    <li>varjolintu (Browser Integration)</li>
    <li>louib (CLI)</li>
    <li>hifi (SSH Agent)</li>
    <li>xvallspl (Tags)</li>
    <li>Aetf (FdoSecrets Storage Server)</li>
    <li>smlu (Visual Studio support)</li>
    <li>ckieschnick (KeeShare)</li>
    <li>seatedscribe (CSV Import)</li>
    <li>weslly (macOS improvements)</li>
    <li>brainplot (many improvements)</li>
    <li>kneitinger (many improvements)</li>
    <li>frostasm (many improvements)</li>
    <li>libklein (many improvements)</li>
    <li>w15eacre (many improvements)</li>
    <li>fonic (Entry Table View)</li>
    <li>kylemanna (YubiKey)</li>
    <li>c4rlo (Offline HIBP Checker)</li>
    <li>wolframroesler (HTML Export, Statistics, Password Health, HIBP integration)</li>
    <li>mdaniel (OpVault Importer)</li>
    <li>angelsl (KDBX 4)</li>
    <li>TheZ3ro (retired lead)</li>
    <li>debfx (KeePassX)</li>
    <li>BlueIce (KeePassX)</li>
</ul>
<h3>Patreon Supporters:</h3>
<ul>
    <li>Shintaro Matsushima</li>
    <li>Paul Ellenbogen</li>
    <li>John C</li>
    <li>Markus</li>
    <li>Crimson Idol</li>
    <li>Steven</li>
    <li>Ellie</li>
    <li>Anthony Avina</li>
    <li>PlushElderGod</li>
    <li>zapscribe</li>
    <li>Christopher Hillenbrand</li>
    <li>Dave Jones</li>
    <li>Brett</li>
    <li>Ralph Azucena</li>
    <li>Florian</li>
    <li>Kristoffer Winther Balling</li>
    <li>Peter Link</li>
    <li>David S H Rosenthal</li>
    <li>Michael Soares</li>
    <li>Vlad Didenko</li>
    <li>henloo</li>
    <li>Erik Rigtorp</li>
    <li>Barry McKenzie</li>
    <li>Sebastian van der Est</li>
    <li>J.C. Polanycia</li>
    <li>Peter Upfold</li>
    <li>Josh Yates-Walker</li>
    <li>Adam</li>
    <li>HJ</li>
    <li>bjorndown</li>
    <li>Tony Wang</li>
    <li>Nol Aders</li>
    <li>Dirk Bergstrom</li>
    <li>proco</li>
    <li>Philipp Baderschneider</li>
    <li>Neša</li>
    <li>Dimitris Kogias</li>
    <li>Robin Hellsten</li>
    <li>Scott Williams</li>
    <li>klepto68</li>
    <li>Uwe S.</li>
    <li>codiflow</li>
    <li>eugene</li>
    <li>Anton Fisher</li>
    <li>David Daly</li>
    <li>Crispy_Steak</li>
    <li>Cilestin</li>
    <li>Benjamin</li>
    <li>Daniel Lakeland</li>
    <li>erinacio</li>
    <li>Leo</li>
    <li>Payton</li>
    <li>Saicxs</li>
    <li>Gorund O</li>
    <li>Tony G</li>
    <li>Simonas S.</li>
    <li>LordKnoppers</li>
    <li>Fabien Duchaussois</li>
    <li>Tim Bahnes</li>
    <li>Aleksei Gusev</li>
    <li>J Hanssen</li>
    <li>schoepp</li>
    <li>Klaus</li>
    <li>Eric</li>
    <li>Griffondale</li>
    <li>Andy D</li>
    <li>YAMAMOTO Yuji</li>
    <li>elmiko</li>
    <li>David</li>
    <li>Nate Wynd</li>
    <li>Nicolas</li>
    <li>magila</li>
    <li>Bryan Fisher</li>
    <li>Mark Nicholson</li>
    <li>Asperatus</li>
    <li>Patrick Buchan-Hepburn</li>
    <li>Richárd Faragó</li>
    <li>David Koch</li>
    <li>cheese_cake</li>
    <li>duke_money</li>
    <li>lund</li>
    <li>Ivana Kellyer</li>
    <li>Skullzam</li>
    <li>Chris Bier</li>
    <li>Gustavo</li>
    <li>Henning_IdB</li>
    <li>Edd</li>
    <li>Net</li>
    <li>Sergei Slipchenko</li>
    <li>Amanita</li>
    <li>Gaara</li>
    <li>Max</li>
    <li>5h4d3</li>
    <li>James Taylor</li>
    <li>Alexei Bond</li>
    <li>cck</li>
    <li>David L</li>
    <li>devNull</li>
    <li>Erica</li>
    <li>Matthew O</li>
    <li>Druggo Yang</li>
    <li>Eric Stokes</li>
</ul>
<h3>GitHub Sponsors:</h3>
<ul>
    <li>rszamszur</li>
    <li>Sidicas</li>
    <li>Mr-NH</li>
</ul>
<h3>Translations:</h3>
<ul>
    <li><strong>Arabic:</strong> kmutahar</li>
    <li><strong>Chinese (China):</strong> Biggulu, Brandon_c, hoilc, ligyxy, Small_Ku, umi_neko, vc5</li>
    <li><strong>Chinese (Taiwan):</strong> BestSteve, flachesis, MiauLightouch, Small_Ku, yan12125, ymhuang0808</li>
    <li><strong>Czech:</strong> DanielMilde, pavelb, tpavelek</li>
    <li><strong>English (United Kingdom):</strong> YCMHARHZ</li>
    <li><strong>English (United States):</strong> alexandercrice, DarkHolme, nguyenlekhtn</li>
    <li><strong>Finnish:</strong> artnay, hif1, MawKKe, varjolintu</li>
    <li><strong>German:</strong> antsas, BasicBaer, Calyrx, codejunky, DavidHamburg, eth0, for1real, jensrutschmann,
        joe776, kflesch, marcbone, MarcEdinger, mcliquid, mfernau77, montilo, nursoda, omnisome4, origin_de, pcrcoding,
        rgloor, vlenzer, waster, Wyrrrd</li>
    <li><strong>Greek:</strong> magkopian, nplatis, tassos.b, xinomilo</li>
    <li><strong>Hungarian:</strong> bubu, meskobalazs, urbalazs</li>
    <li><strong>Indonesian:</strong> zk</li>
    <li><strong>Italian:</strong> amaxis, duncanmid, FranzMari, lucaim, NITAL, Peo, tosky, VosaxAlo</li>
    <li><strong>Japanese:</strong> masoo, p2635, Shinichirou_Yamada, vmemjp, yukinakato</li>
    <li><strong>Korean:</strong> cancantun, peremen</li>
    <li><strong>Lithuanian:</strong> Moo</li>
    <li><strong>Norwegian Bokmål:</strong> eothred, haarek, torgeirf</li>
    <li><strong>Polish:</strong> keypress, mrerexx, psobczak</li>
    <li><strong>Portuguese (Brazil):</strong> danielbibit, fabiom, flaviobn, newmanisaac, vitor895, weslly</li>
    <li><strong>Portuguese (Portugal):</strong> a.santos, American_Jesus, hds, lmagomes, smarquespt</li>
    <li><strong>Romanian:</strong> alexminza</li>
    <li><strong>Russian:</strong> _nomoretears_, agag11507, alexminza, anm, artemkonenko, denoos, KekcuHa, Mogost,
        netforhack, NetWormKido, RKuchma, ShareDVI, talvind, VictorR2007, vsvyatski, wkill95</li>
    <li><strong>Serbian:</strong> ArtBIT</li>
    <li><strong>Swedish:</strong> Anders_Bergqvist, henziger, jpyllman, peron, Thelin</li>
    <li><strong>Turkish:</strong> etc, N3pp</li>
    <li><strong>Ukrainian:</strong> brisk022, netforhack, ShareDVI, zoresvit</li>
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

    m_ui->iconLabel->setPixmap(icons()->applicationIcon().pixmap(48));

    QString debugInfo = Tools::debugInfo().append("\n").append(Crypto::debugInfo());
    m_ui->debugInfo->setPlainText(debugInfo);

    m_ui->maintainers->setText(aboutMaintainers);
    m_ui->contributors->setText(aboutContributors);

    setAttribute(Qt::WA_DeleteOnClose);
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->copyToClipboard, SIGNAL(clicked()), SLOT(copyToClipboard()));

    m_ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
}

AboutDialog::~AboutDialog() = default;

void AboutDialog::copyToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(m_ui->debugInfo->toPlainText());
}
