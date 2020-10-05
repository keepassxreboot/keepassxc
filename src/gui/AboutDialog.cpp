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
    <li>Chris Sohns</li>
    <li>Kyle Kneitinger</li>
    <li>Sergey Vilgelm</li>
    <li>Roman Vaughan (NZSmartie)</li>
    <li>Shmavon Gazanchyan</li>
    <li>Riley Moses</li>
    <li>Korbinian Schildmann</li>
    <li>Andreas (nitrohorse)</li>
    <li>Kernellinux</li>
    <li>Micha Ober</li>
    <li>PublicByte</li>
</ul>
<h3>Notable Code Contributions:</h3>
<ul>
    <li>droidmonkey</li>
    <li>phoerious</li>
    <li>louib (CLI)</li>
    <li>varjolintu (Browser Integration)</li>
    <li>hifi (SSH Agent)</li>
    <li>ckieschnick (KeeShare)</li>
    <li>seatedscribe (CSV Import)</li>
    <li>Aetf (FdoSecrets Storage Server)</li>
    <li>weslly (macOS improvements)</li>
    <li>brainplot (many improvements)</li>
    <li>kneitinger (many improvements)</li>
    <li>frostasm (many improvements)</li>
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
    <li>Igor Zinovik</li>
    <li>Alexanderjb</li>
    <li>Richard Ames</li>
    <li>SLmanDR</li>
    <li>Christian Rasmussen</li>
    <li>Tyler Gass</li>
    <li>Nuutti Toivola</li>
    <li>Gregory Werbin</li>
    <li>Lionel LaskÃ©</li>
    <li>Ivar</li>
    <li>Darren</li>
    <li>Brad</li>
    <li>Mathieu Peltier</li>
    <li>gonczor</li>
    <li>Oleksii Aleksieiev</li>
    <li>Gernot Premper</li>
    <li>Julian Stier</li>
    <li>Daniel Epp</li>
    <li>Ruben Schade</li>
    <li>Esteban Martinez</li>
    <li>Niels Ganser</li>
    <li>turin231</li>
    <li>judd</li>
    <li>Tarek Sherif</li>
    <li>Bernhard</li>
    <li>William Komanetsky</li>
    <li>Clark Henry</li>
    <li>Justin Carroll</li>
    <li>Shintaro Matsushima</li>
    <li>Larry Siden</li>
    <li>Thammachart Chinvarapon</li>
    <li>Patrick Evans</li>
</ul>
<h3>Translations:</h3>
<ul>
    <li><strong>العربية (Arabic)</strong>: AboShanab, kmutahar, muha_abdulaziz, Night1, omar.nsy</li>
    <li><strong>euskara (Basque)</strong>: azken_tximinoa, Galaipa, Hey_neken</li>
    <li><strong>বাংলা (Bengali)</strong>: codesmite</li>
    <li><strong>ဗမာစာ (Burmese)</strong>: Snooooowwwwwman</li>
    <li><strong>català (Catalan)</strong>: antoniopolonio, capitantrueno, dsoms, MarcRiera, mcus, raulua, ZJaume</li>
    <li><strong>中文 (Chinese (Simplified))</strong>: Biggulu, Brandon_c, carp0129, Dy64, ef6, Felix2yu, hoilc, ivlioioilvi,
        kikyous, kofzhanganguo, ligyxy, lxx4380, remonli, ShuiHuo, slgray, Small_Ku, snhun, umi_neko, vc5, Wylmer_Wang, Z4HD</li>
    <li><strong>中文 (台灣) (Chinese (Traditional))</strong>: BestSteve, flachesis, gojpdchx, ligyxy, MiauLightouch, plesry,
        priv, raymondtau, Small_Ku, th3lusive, yan12125, ymhuang0808</li>
    <li><strong>hrvatski jezik (Croatian)</strong>: Halberd, mladenuzelac</li>
    <li><strong>čeština (Czech)</strong>: awesomevojta, DanielMilde, JosefVitu, pavelb, stps, tpavelek</li>
    <li><strong>dansk (Danish)</strong>: alfabetacain, ebbe, GimliDk, JakobPP, KalleDK, MannVera, nlkl, thniels</li>
    <li><strong>Nederlands (Dutch)</strong>: apie, bartlibert, Bubbel, bython, Dr.Default, e2jk, evanoosten, fourwood,
        fvw, glotzbach, JCKalman, KnooL, ovisicnarf, pietermj, rigrig, srgvg, Stephan_P, stijndubrul, theniels17,
        ThomasChurchman, Vistaus, wanderingidea, Zombaya1</li>
    <li><strong>English (UK)</strong>: CisBetter, rookwood101, spacemanspiff, throne3d, YCMHARHZ</li>
    <li><strong>English (USA)</strong>: alexandercrice, caralu74, cl0ne, DarkHolme, nguyenlekhtn, thedoctorsoad, throne3d</li>
    <li><strong>Esperanto (Esperanto)</strong>: batisteo</li>
    <li><strong>eesti (Estonian)</strong>: Hermanio</li>
    <li><strong>suomi (Finnish)</strong>: artnay, hif1, MawKKe, petri, tomisalmi, varjolintu</li>
    <li><strong>français (French)</strong>: A1RO, aghilas.messara, Albynton, alexisju, b_mortgat, Beatussum, benoitbalon,
        bertranoel, bisaloo, Cabirto, Code2Mirabeau, e2jk, ebrious, frgnca, Fumble, ggtr1138, gilbsgilbs, gohuros, gtalbot,
        Gui13, houdini, houdini69, iannick, jlutran, John.Mickael, kyodev, lacnic, laetilodie, logut, MartialBis, Maxime_J,
        mlpo, Morgan, MrHeadwar, narzb, nekopep, Nesousx, pBouillon, Raphi111, Scrat15, TheFrenchGhosty, theodex, tl_pierre,
        webafrancois, wilfriedroset, yahoe.001, zedentox</li>
    <li><strong>Galego (Galician)</strong>: enfeitizador</li>
    <li><strong>Deutsch (German)</strong>: andreas.maier, antsas, Atalanttore, BasicBaer, bwolkchen, Calyrx, codejunky,
        DavidHamburg, derhagen, eth0, fahstat, for1real, Gyges, Hativ, hjonas, HoferJulian, janis91, jensrutschmann,
        joe776, kflesch, man_at_home, marcbone, MarcEdinger, markusd112, Maxime_J, mbetz, mcliquid, mfernau77, mircsicz,
        mithrial, montilo, MuehlburgPhoenix, muellerma, nautilusx, Nerzahd, Nightwriter, NotAName, nursoda, omnisome4,
        origin_de, pcrcoding, PFischbeck, rgloor, rugk, ScholliYT, Silas_229, spacemanspiff, testarossa47, TheForcer,
        transi_222, traschke, vlenzer, vpav, waster, wolfram.roesler, Wyrrrd</li>
    <li><strong>ελληνικά (Greek)</strong>: anvo, magkopian, nplatis, tassos.b, xinomilo</li>
    <li><strong>עברית (Hebrew)</strong>: shmag18</li>
    <li><strong>magyar (Hungarian)</strong>: andras_tim, bubu, meskobalazs, urbalazs</li>
    <li><strong>Íslenska (Icelandic)</strong>: MannVera</li>
    <li><strong>Bahasa (Indonesian)</strong>: achmad, bora_ach, zk</li>
    <li><strong>Italiano (Italian)</strong>: amaxis, bovirus, duncanmid, FranzMari, Gringoarg, lucaim, NITAL, Peo,
        salvatorecordiano, seatedscribe, Stemby, the.sailor, tosky, VosaxAlo</li>
    <li><strong>日本語 (Japanese)</strong>: gojpdchx, masoo, metalic_cat, p2635, saita, Shinichirou_Yamada, take100yen,
        Umoxfo, vargas.peniel, vmemjp, WatanabeShint, yukinakato</li>
    <li><strong>қазақ тілі (Kazakh)</strong>: sotrud_nik</li>
    <li><strong>한국어 (Korean)</strong>: cancantun, peremen</li>
    <li><strong>latine (Latin)</strong>: alexandercrice</li>
    <li><strong>lietuvių kalba (Lithuanian)</strong>: Moo, pauliusbaulius, rookwood101</li>
    <li><strong>Norsk Bokmål (Norwegian Bokmål)</strong>: eothred, haarek, JardarBolin, jumpingmushroom, sattor, torgeirf,
        ysteinalver</li>
    <li><strong>język polski (Polish)</strong>: AreYouLoco, dedal123, hoek, keypress, konradmb, mrerexx, pabli, psobczak,
        SebJez</li>
    <li><strong>Português (Portuguese)</strong>: weslly, xendez</li>
    <li><strong>Português (Portuguese (Brazil))</strong>: andersoniop, danielbibit, fabiom, flaviobn, guilherme__sr,
        Havokdan, lucasjsoliveira, mauri.andres, newmanisaac, rafaelnp, RockyTV, vitor895, weslly, xendez</li>
    <li><strong>Português (Portuguese (Portugal))</strong>: a.santos, American_Jesus, arainho, hds, lmagomes, mihai.ile,
        pfialho, smarquespt, smiguel, xendez, xnenjm</li>
    <li><strong>Română (Romanian)</strong>: alexminza, drazvan, polearnik</li>
    <li><strong>русский (Russian)</strong>: _nomoretears_, agag11507, alexminza, anm, artemkonenko, cl0ne, denoos, DG,
        JayDi85, KekcuHa, Mogost, Mr.GreyWolf, MustangDSG, NcNZllQnHVU, netforhack, NetWormKido, Rakleed, RKuchma,
        ruslan.denisenko, ShareDVI, Shevchuk, solodyagin, talvind, VictorR2007, vsvyatski, wkill95</li>
    <li><strong>српски језик (Serbian)</strong>: ArtBIT, oros</li>
    <li><strong>Slovenčina (Slovak)</strong>: Asprotes, crazko, l.martinicky, pecer, Slavko</li>
    <li><strong>Español (Spanish)</strong>: adolfogc, AdrianClv, AndreachongB, AndresQ, antifaz, Bendhet, capitantrueno,
        caralu74, DarkHolme, e2jk, EdwardNavarro, eliluminado, erinm, gonrial, iglpdc, jojobrambs, LeoBeltran, lupa18,
        masanchez5000, mauri.andres, NicolasCGN, Pablohn, pdinoto, picodotdev, piegope, pquin, puchrojo, rcalpha,
        rodolfo.guagnini, systurbed, vargas.peniel, ventolinmono, vsvyatski, Xlate1984, zmzpa, Zranz</li>
    <li><strong>Svenska (Swedish)</strong>: 0x9fff00, Anders_Bergqvist, ArmanB, baxtex, eson, henziger, jpyllman, krklns,
        LIINdd, malkus, peron, Thelin, theschitz, victorhggqvst, zeroxfourc</li>
    <li><strong>ไทย (Thai)</strong>: arthit, ben_cm, chumaporn.t, darika, digitalthailandproject, GitJirasamatakij,
        muhammadmumean, nipattra, ordinaryjane, rayg, sirawat, Socialister, Wipanee</li>
    <li><strong>Türkçe (Turkish)</strong>: cagries, etc, ethem578, mcveri, N3pp, SeLeNLeR, TeknoMobil, Ven_Zallow</li>
    <li><strong>Українська (Ukrainian)</strong>: brisk022, chulivska, cl0ne, exlevan, m0stik, netforhack, paul_sm, ShareDVI,
        zoresvit</li>
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

AboutDialog::~AboutDialog()
{
}

void AboutDialog::copyToClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(m_ui->debugInfo->toPlainText());
}
