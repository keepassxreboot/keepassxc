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

#include "config-keepassx.h"
#include "DatabaseIcons.h"

DatabaseIcons* DatabaseIcons::m_instance(0);

QIcon DatabaseIcons::icon(int index)
{
    if (index >= iconCount()) {
        return QIcon();
    }

    if (!m_instance) {
        m_instance = new DatabaseIcons();
    }

    return m_instance->getIconInternal(index);
}

int DatabaseIcons::iconCount()
{
    return 69;
}

DatabaseIcons::DatabaseIcons()
{
    m_indexToName.append("C00_Password.png");
    m_indexToName.append("C01_Package_Network.png");
    m_indexToName.append("C02_MessageBox_Warning.png");
    m_indexToName.append("C03_Server.png");
    m_indexToName.append("C04_Klipper.png");
    m_indexToName.append("C05_Edu_Languages.png");
    m_indexToName.append("C06_KCMDF.png");
    m_indexToName.append("C07_Kate.png");
    m_indexToName.append("C08_Socket.png");
    m_indexToName.append("C09_Identity.png");
    m_indexToName.append("C10_Kontact.png");
    m_indexToName.append("C11_Camera.png");
    m_indexToName.append("C12_IRKickFlash.png");
    m_indexToName.append("C13_KGPG_Key3.png");
    m_indexToName.append("C14_Laptop_Power.png");
    m_indexToName.append("C15_Scanner.png");
    m_indexToName.append("C16_Mozilla_Firebird.png");
    m_indexToName.append("C17_CDROM_Unmount.png");
    m_indexToName.append("C18_Display.png");
    m_indexToName.append("C19_Mail_Generic.png");
    m_indexToName.append("C20_Misc.png");
    m_indexToName.append("C21_KOrganizer.png");
    m_indexToName.append("C22_ASCII.png");
    m_indexToName.append("C23_Icons.png");
    m_indexToName.append("C24_Connect_Established.png");
    m_indexToName.append("C25_Folder_Mail.png");
    m_indexToName.append("C26_FileSave.png");
    m_indexToName.append("C27_NFS_Unmount.png");
    m_indexToName.append("C28_QuickTime.png");
    m_indexToName.append("C29_KGPG_Term.png");
    m_indexToName.append("C30_Konsole.png");
    m_indexToName.append("C31_FilePrint.png");
    m_indexToName.append("C32_FSView.png");
    m_indexToName.append("C33_Run.png");
    m_indexToName.append("C34_Configure.png");
    m_indexToName.append("C35_KRFB.png");
    m_indexToName.append("C36_Ark.png");
    m_indexToName.append("C37_KPercentage.png");
    m_indexToName.append("C38_Samba_Unmount.png");
    m_indexToName.append("C39_History.png");
    m_indexToName.append("C40_Mail_Find.png");
    m_indexToName.append("C41_VectorGfx.png");
    m_indexToName.append("C42_KCMMemory.png");
    m_indexToName.append("C43_EditTrash.png");
    m_indexToName.append("C44_KNotes.png");
    m_indexToName.append("C45_Cancel.png");
    m_indexToName.append("C46_Help.png");
    m_indexToName.append("C47_KPackage.png");
    m_indexToName.append("C48_Folder.png");
    m_indexToName.append("C49_Folder_Blue_Open.png");
    m_indexToName.append("C50_Folder_Tar.png");
    m_indexToName.append("C51_Decrypted.png");
    m_indexToName.append("C52_Encrypted.png");
    m_indexToName.append("C53_Apply.png");
    m_indexToName.append("C54_Signature.png");
    m_indexToName.append("C55_Thumbnail.png");
    m_indexToName.append("C56_KAddressBook.png");
    m_indexToName.append("C57_View_Text.png");
    m_indexToName.append("C58_KGPG.png");
    m_indexToName.append("C59_Package_Development.png");
    m_indexToName.append("C60_KFM_Home.png");
    m_indexToName.append("C61_Services.png");
    m_indexToName.append("C62_Tux.png");
    m_indexToName.append("C63_Feather.png");
    m_indexToName.append("C64_Apple.png");
    m_indexToName.append("C65_Apple.png");
    m_indexToName.append("C66_Money.png");
    m_indexToName.append("C67_Certificate.png");
    m_indexToName.append("C68_BlackBerry.png");

    Q_ASSERT(m_indexToName.size() == iconCount());
}
#include <QFile>
QIcon DatabaseIcons::getIconInternal(int index)
{
    if (m_iconCache.contains(index)) {
        return m_iconCache.value(index);
    }
    else {
        // TODO search multiple paths
        QIcon icon(QString(KEEPASSX_SOURCE_DIR).append("/share/icons/database/").append(m_indexToName.at(index)));

        m_iconCache.insert(index, icon);
        return icon;
    }
}
