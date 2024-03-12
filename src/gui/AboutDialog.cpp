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
    <li>Caleb Currie</li>
    <li>Morgan Courbet</li>
    <li>Kyle Kneitinger</li>
    <li>Chris Sohns</li>
    <li>Shmavon Gazanchyan</li>
    <li>xjdwc</li>
    <li>Riley Moses</li>
    <li>Igor Zinovik</li>
    <li>Jeff</li>
    <li>Esteban Martinez</li>
    <li>Max Andersen</li>
    <li>Zivix</li>
    <li>Marc Morocutti</li>
    <li>super scampy</li>
    <li>Hugo Locurcio</li>
    <li>Benedikt Heine</li>
    <li>Mischa Peters</li>
    <li>Rainer-Maria Fritsch</li>
    <li>Micha Ober</li>
    <li>Ivan Gromov</li>
    <li>William Petrides</li>
    <li>Joshua Go</li>
    <li>Gunar Gessner</li>
    <li>pancakeplant</li>
    <li>Hans-Joachim Forker</li>
    <li>Nicolas Vandemaele</li>
    <li>Saturnio</li>
    <li>Robert Schaffar-Taurok</li>
    <li>Mike</li>
    <li>Thomas Renz</li>
    <li>Toby Cline</li>
    <li>Christian Wittenhorst</li>
    <li>Paul Ammann</li>
    <li>Matt Cardarelli</li>
    <li>Steve Isom</li>
    <li>Emre Dessoi</li>
    <li>Wojciech Kozlowski</li>
    <li>Michael Babnick</li>
    <li>kernellinux</li>
    <li>Patrick Evans</li>
    <li>Marco</li>
    <li>GodSpell</li>
    <li>Jeremy Rubin</li>
    <li>Korbi</li>
    <li>andreas</li>
    <li>Tyche's tidings</li>
    <li>Daniel Kuebler</li>
    <li>Brandon Corujo</li>
</ul>
<h3>Notable Code Contributions:</h3>
<ul>
    <li>droidmonkey</li>
    <li>phoerious</li>
    <li>louib (CLI)</li>
    <li>varjolintu (Browser Integration)</li>
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
    <li>Richard Ames</li>
    <li>Bernhard</li>
    <li>Christian Rasmussen</li>
    <li>Nuutti Toivola</li>
    <li>Lionel Laské</li>
    <li>Tyler Gass</li>
    <li>NZSmartie</li>
    <li>Darren</li>
    <li>Brad</li>
    <li>Oleksii Aleksieiev</li>
    <li>Julian Stier</li>
    <li>Daniel Epp</li>
    <li>Ruben Schade</li>
    <li>William Komanetsky</li>
    <li>Niels Ganser</li>
    <li>judd</li>
    <li>Tarek Sherif</li>
    <li>Eugene</li>
    <li>CYB3RL4MBD4</li>
    <li>Alexanderjb</li>
    <li>Justin Carroll</li>
    <li>Bart Libert</li>
    <li>Shintaro Matsushima</li>
    <li>Thammachart Chinvarapon</li>
    <li>Gernot Premper</li>
    <li>SLmanDR</li>
    <li>Paul Ellenbogen</li>
    <li>John C</li>
    <li>Markus Wochnik</li>
    <li>Clark Henry</li>
    <li>zapscribe</li>
    <li>Salt Rock Lamp</li>
    <li>Steven Crowley</li>
    <li>Ralph Azucena</li>
    <li>Guruprasad Kulkarni</li>
    <li>jose</li>
    <li>Michael Gulick</li>
    <li>J Doty</li>
    <li>Synchro11</li>
    <li>Michael Soares</li>
    <li>Johannes Felko</li>
    <li>Ellie</li>
    <li>David Walluscheck</li>
    <li>Anthony Avina</li>
    <li>pro</li>
    <li>Mark Luxton</li>
    <li>Crimson Idol</li>
    <li>Björn König</li>
    <li>René Weselowski</li>
    <li>gonczor</li>
    <li>PlushElderGod</li>
    <li>gilgwath</li>
    <li>Tobias</li>
    <li>Christopher Hillenbrand</li>
    <li>Daddy's c$sh</li>
    <li>Ashura</li>
    <li>Florian</li>
    <li>Alexandre</li>
    <li>Dave Jones</li>
    <li>Brett</li>
    <li>Jim Vanderbilt</li>
    <li>Brian McGuire</li>
    <li>Sid Beske</li>
    <li>Dmitrii Galinskii</li>
    <li>Johannes Erchen</li>
    <li>Brandon Zhang</li>
    <li>Maxley Fraser</li>
    <li>Nikul Savasadia</li>
    <li>Claude</li>
    <li>alga</li>
    <li>Philipp Jetschina</li>
    <li>Kristoffer Winther Balling</li>
    <li>Peter Link</li>
    <li>Vlastimil Vondra</li>
    <li>Tony Wang</li>
    <li>John Sivak</li>
    <li>Nol Aders</li>
    <li>Charlie Drake</li>
    <li>Ryan Goldstein</li>
    <li>Doug Witt</li>
    <li>David S H Rosenthal</li>
    <li>Lance Simmons</li>
    <li>Mathew Woodyard</li>
    <li>GanderPL</li>
    <li>Neša</li>
    <li>tolias</li>
    <li>Adam</li>
</ul>
<h3>Translations:</h3>
<ul>
    <li><strong>العربية (Arabic)</strong>: 3eani, 3nad, AboShanab, butterflyoffire_temp, jBailony, kmutahar, m.hemoudi,
        Marouane87, microtaha, mohame1d, muha_abdulaziz, Night1, omar.nsy, TheAhmed, zer0x</li>
    <li><strong>euskara (Basque)</strong>: azken_tximinoa, Galaipa, Hey_neken, porrumentzio</li>
    <li><strong>বাংলা (Bengali)</strong>: codesmite, Foxom, rediancool, RHJihan</li>
    <li><strong>ဗမာစာ (Burmese)</strong>: Christine.Ivy, hafe14, Snooooowwwwwman, tuntunaung</li>
    <li><strong>català (Catalan)</strong>: antoniopolonio, benLabcat, capitantrueno, dsoms, Ecron, jamalinu, jmaribau,
        MarcRiera, mcus, raulua, zeehio, ZJaume</li>
    <li><strong>中文 (Chinese (Simplified))</strong>: Biacke, Biggulu, Brandon_c, carp0129, Clafiok, deluxghost, Dy64,
        ef6, Felix2yu, hoilc, jy06308127, kikyous, kofzhanganguo, ligyxy, lxx4380, oksjd, remonli, ShuiHuo, sjdhome,
        slgray, Small_Ku, snhun, umi_neko, vc5, Wylmer_Wang, Z4HD</li>
    <li><strong>中文 (台灣) (Chinese (Traditional))</strong>: BestSteve, Biacke, flachesis, gojpdchx, ligyxy, MiauLightouch,
        plesry, priv, raymondtau, Siriusmart, Small_Ku, ssuhung, Superbil, th3lusive, yan12125, ymhuang0808</li>
    <li><strong>hrvatski jezik (Croatian)</strong>: krekrekre, mladenuzelac</li>
    <li><strong>čeština (Czech)</strong>: DanielMilde, jiri.jagos, pavelb, pavelz, S474N, stps, tpavelek, vojtechjurcik</li>
    <li><strong>dansk (Danish)</strong>: alfabetacain, dovecode, ebbe, ERYpTION, GimliDk, Grooty12, JakobPP, KalleDK,
        MannVera, nlkl, Saustrup, thniels</li>
    <li><strong>Nederlands (Dutch)</strong>: apie, bartlibert, Bubbel, bython, Dr.Default, e2jk, evanoosten, fourwood,
        fvw, glotzbach, JCKalman, keunes, KnooL, ms.vd.linden, ovisicnarf, pietermj, pvdl, rigrig, srgvg, Stephan_P,
        stijndubrul, theniels17, ThomasChurchman, timschreinemachers, Vistaus, wanderingidea, Zombaya1</li>
    <li><strong>Esperanto (Esperanto)</strong>: batisteo</li>
    <li><strong>eesti (Estonian)</strong>: Hermanio, okul, sarnane, tlend, V6lur</li>
    <li><strong>suomi (Finnish)</strong>: artnay, hif1, MawKKe, petri, tomisalmi, uusijani, varjolintu</li>
    <li><strong>français (French)</strong>: ayiniho, Beatussum, butterflyoffire_temp, Cabirto, francoisa, iannick,
        jean_pierre_raumer, John.Mickael, Jojo6375, lacnic, Marouane87, mohame1d, orion78fr, stephanecodes, swarmpan,
        t0mmy742, TakeÃ§i, tenzap, webafrancois, x0rld</li>
    <li><strong>Galego (Galician)</strong>: damufo, enfeitizador, mustek</li>
    <li><strong>Deutsch (German)</strong>: andreas.maier, antsas, archer_321, ASDFGamer, Atalanttore, BasicBaer, blacksn0w,
        bwolkchen, Calyrx, clonejo, codejunky, DavidHamburg, eth0, fahstat, FlotterCodername, for1real, frlan, funny0facer,
        Gyges, h_h, Hativ, heynemax, hjonas, HoferJulian, hueku, janis91, jensrutschmann, jhit, joe776, kflesch, man_at_home,
        marcbone, MarcEdinger, markusd112, Marouane87, maxwxyz, mcliquid, mfernau77, mircsicz, montilo, MuehlburgPhoenix,
        muellerma, nautilusx, neon64, Nerzahd, Nightwriter, noodles101, NotAName, nursoda, OLLI_S, omnisome4, origin_de,
        pcrcoding, PFischbeck, phallobst, philje, pqtjhhBzDd5NuJ9, r3drock, rakekniven, revoltek, rgloor, Rheggie, RogueThorn,
        rugk, ScholliYT, scotwee, Silas_229, spacemanspiff, SteffoSpieler, testarossa47, TheForcer, thillux, transi_222, traschke,
        Unkn0wnCat, vlenzer, vpav, waster, wolfram.roesler, Wyrrrd, xf</li>
    <li><strong>ελληνικά (Greek)</strong>: anvo, arttor, Dkafetzis, giwrgosmant, GorianM, Jason_M, magkopian, nplatis, saglogog,
        tassos.b, xinomilo</li>
    <li><strong>עברית (Hebrew)</strong>: avimar, ronyala, shemeshg, shmag18, ThunderB0lt, tryandtry, ztwersky</li>
    <li><strong>magyar (Hungarian)</strong>: andras_tim, bubu, entaevau, kempelen, meskobalazs, spammy, typingseashell, urbalazs</li>
    <li><strong>Íslenska (Icelandic)</strong>: MannVera</li>
    <li><strong>Bahasa Indonesia (Indonesian)</strong>: achmad, algustionesa, bora_ach, racrbmr, zk</li>
    <li><strong>Italiano (Italian)</strong>: aleb2000, amaxis, bovirus, duncanmid, FranzMari, Gringoarg, idetao, lucaim, NITAL, Peo,
        Pietrog, salvatorecordiano, seatedscribe, Stemby, the.sailor, tosky, VosaxAlo</li>
    <li><strong>日本語 (Japanese)</strong>: AlCooo, gojpdchx, helloguys, masoo, p2635, Shinichirou_Yamada, shortarrow, ssuhung, tadasu,
        take100yen, Umoxfo, vargas.peniel, vmemjp, WatanabeShint, yukinakato</li>
    <li><strong>қазақ тілі (Kazakh)</strong>: sotrud_nik</li>
    <li><strong>한국어 (Korean)</strong>: BraINstinct0, cancantun, peremen</li>
    <li><strong>latine (Latin)</strong>: alexandercrice</li>
    <li><strong>latviešu valoda (Latvian)</strong>: andis.luksho, victormeirans, wakeeshi</li>
    <li><strong>lietuvių kalba (Lithuanian)</strong>: Kornelijus, Moo, pauliusbaulius, rookwood101, wakeeshi</li>
    <li><strong>Norsk Bokmål (Norwegian Bokmål)</strong>: bkvamme, eirikl, eothred, haarek, JardarBolin, jumpingmushroom, sattor,
        torgeirf, ysteinalver</li>
    <li><strong>ਪੰਜਾਬੀ (Punjabi)</strong>: aalam</li>
    <li><strong>فارسی (Farsi)</strong>: gnulover, siamax</li>
    <li><strong>فارسی (Farsi (Iran))</strong>: magnifico</li>
    <li><strong>język polski (Polish)</strong>: AreYouLoco, dedal123, EsEnZeT, hoek, keypress, konradmb, mrerexx, pabli, ply,
        psobczak, SebJez, verahawk</li>
    <li><strong>Português (Portuguese)</strong>: diraol, hugok, pfialho, rudahximenes, weslly, xendez</li>
    <li><strong>Português (Portuguese (Brazil))</strong>: alinda, amalvarenga, andersoniop, danielbibit, diraol, fabiom, flaviobn,
        fmilagres, furious_, gabrieljcs, Guilherme.Peev, guilherme__sr, Havokdan, igorruckert, josephelias94, keeBR, kiskadee, lecalam,
        lucasjsoliveira, mauri.andres, newmanisaac, rafaelnp, ruanmed, rudahximenes, ul1sses, vitor895, weslly, wtuemura, xendez,
        zodSilence</li>
    <li><strong>Português (Portuguese (Portugal))</strong>: a.santos, American_Jesus, arainho, hds, hugok, lecalam, lmagomes, pfialho,
        smarquespt, smiguel, xendez, xnenjm</li>
    <li><strong>Română (Romanian)</strong>: _parasite_, aduzsardi, alexminza, polearnik</li>
    <li><strong>русский (Russian)</strong>: 3nad, _nomoretears_, agag11507, alexandersokol, alexminza, anm, artemkonenko, ashed,
        BANOnotIT, burningalchemist, cl0ne, cnide, denoos, DG, DmitriyMaksimov, egorrabota, injseon, Japet, JayDi85, KekcuHa, kerastinell,
        laborxcom, leo9uinuo98, Mogost, Mr.GreyWolf, MustangDSG, netforhack, NetWormKido, nibir, Olesya_Gerasimenko, onix, Orianti,
        RKuchma, ruslan.denisenko, ShareDVI, Shevchuk, solodyagin, talvind, treylav, upupa, VictorR2007, vsvyatski, wakeeshi, Walter.S,
        wkill95, wtigga, zOrg1331</li>
    <li><strong>српски језик (Serbian)</strong>: ArtBIT, ozzii</li>
    <li><strong>Slovenčina (Slovak)</strong>: Asprotes, crazko, jose1711, l.martinicky, pecer, reisuya, Slavko</li>
    <li><strong>Slovenščina (Slovenian)</strong>: asasdasd, samodekleva</li>
    <li><strong>Español (Spanish)</strong>: adolfogc, antifaz, capitantrueno, cquike, cyphra, DarkHolme, doubleshuffle, e2jk,
        EdwardNavarro, fserrador, gabeweb, gonrial, jjtp, jorpilo, LeoBeltran, mauri.andres, piegope, pquin, puchrojo, rodolfo.guagnini,
        tierracomun, vsvyatski</li>
    <li><strong>Svenska (Swedish)</strong>: 0x9fff00, aiix, Anders_Bergqvist, ArmanB, Autom, baxtex, eson, henziger, jpyllman, malkus,
        merikan, peron, peterkz, Thelin, theschitz, victorhggqvst</li>
    <li><strong>ไทย (Thai)</strong>: arthit, ben_cm, chumaporn.t, darika, digitalthailandproject, GitJirasamatakij, ll3an, minoplhy,
        muhammadmumean, nimid, nipattra, ordinaryjane, rayg, sirawat, Socialister, Wipanee</li>
    <li><strong>Türkçe (Turkish)</strong>: abcmen, ahmed.ulusoy, cagries, denizoglu, desc4rtes, etc, ethem578, kayazeren, mcveri, N3pp,
        rgucluer, SeLeNLeR, sprlptr48, TeknoMobil, Ven_Zallow, veysiertekin</li>
    <li><strong>Українська (Ukrainian)</strong>: brisk022, chulivska, cl0ne, exlevan, m0stik, moudrick, netforhack, olko, onix, paul_sm,
        ShareDVI, upupa, zoresvit</li>
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
