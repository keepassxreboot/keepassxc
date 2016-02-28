/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "DatabaseIcons.h"

#include "core/FilePath.h"

DatabaseIcons* DatabaseIcons::m_instance(nullptr);
const int DatabaseIcons::IconCount(69);
const int DatabaseIcons::ExpiredIconIndex(45);
const char* const DatabaseIcons::m_indexToName[] = {
    "C00_Password.png",
    "C01_Package_Network.png",
    "C02_MessageBox_Warning.png",
    "C03_Server.png",
    "C04_Klipper.png",
    "C05_Edu_Languages.png",
    "C06_KCMDF.png",
    "C07_Kate.png",
    "C08_Socket.png",
    "C09_Identity.png",
    "C10_Kontact.png",
    "C11_Camera.png",
    "C12_IRKickFlash.png",
    "C13_KGPG_Key3.png",
    "C14_Laptop_Power.png",
    "C15_Scanner.png",
    "C16_Mozilla_Firebird.png",
    "C17_CDROM_Unmount.png",
    "C18_Display.png",
    "C19_Mail_Generic.png",
    "C20_Misc.png",
    "C21_KOrganizer.png",
    "C22_ASCII.png",
    "C23_Icons.png",
    "C24_Connect_Established.png",
    "C25_Folder_Mail.png",
    "C26_FileSave.png",
    "C27_NFS_Unmount.png",
    "C28_QuickTime.png",
    "C29_KGPG_Term.png",
    "C30_Konsole.png",
    "C31_FilePrint.png",
    "C32_FSView.png",
    "C33_Run.png",
    "C34_Configure.png",
    "C35_KRFB.png",
    "C36_Ark.png",
    "C37_KPercentage.png",
    "C38_Samba_Unmount.png",
    "C39_History.png",
    "C40_Mail_Find.png",
    "C41_VectorGfx.png",
    "C42_KCMMemory.png",
    "C43_EditTrash.png",
    "C44_KNotes.png",
    "C45_Cancel.png",
    "C46_Help.png",
    "C47_KPackage.png",
    "C48_Folder.png",
    "C49_Folder_Blue_Open.png",
    "C50_Folder_Tar.png",
    "C51_Decrypted.png",
    "C52_Encrypted.png",
    "C53_Apply.png",
    "C54_Signature.png",
    "C55_Thumbnail.png",
    "C56_KAddressBook.png",
    "C57_View_Text.png",
    "C58_KGPG.png",
    "C59_Package_Development.png",
    "C60_KFM_Home.png",
    "C61_Services.png",
    "C62_Tux.png",
    "C63_Feather.png",
    "C64_Apple.png",
    "C65_W.png",
    "C66_Money.png",
    "C67_Certificate.png",
    "C68_BlackBerry.png"
};

QImage DatabaseIcons::icon(int index)
{
    if (index < 0 || index >= IconCount) {
        qWarning("DatabaseIcons::icon: invalid icon index %d", index);
        return QImage();
    }

    if (!m_iconCache[index].isNull()) {
        return m_iconCache[index];
    }
    else {
        QString iconPath = QString("icons/database/").append(m_indexToName[index]);
        QImage icon(filePath()->dataPath(iconPath));

        m_iconCache[index] = icon;
        return icon;
    }
}

QPixmap DatabaseIcons::iconPixmap(int index)
{
    if (index < 0 || index >= IconCount) {
        qWarning("DatabaseIcons::iconPixmap: invalid icon index %d", index);
        return QPixmap();
    }

    QPixmap pixmap;

    if (!QPixmapCache::find(m_pixmapCacheKeys[index], &pixmap)) {
        pixmap = QPixmap::fromImage(icon(index));
        m_pixmapCacheKeys[index] = QPixmapCache::insert(pixmap);
    }

    return pixmap;
}

DatabaseIcons::DatabaseIcons()
{
    Q_STATIC_ASSERT(sizeof(m_indexToName) == IconCount * sizeof(m_indexToName[0]));

    m_iconCache.reserve(IconCount);
    m_iconCache.resize(IconCount);
    m_pixmapCacheKeys.reserve(IconCount);
    m_pixmapCacheKeys.resize(IconCount);
}

DatabaseIcons* DatabaseIcons::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseIcons();
    }

    return m_instance;
}
