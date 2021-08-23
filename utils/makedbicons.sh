#!/usr/bin/env bash
# Assemble database icons from the icons8 collection
# by Wolfram Rösler 2020-04-30

NC='\033[0m'
RED='\033[0;31m'
YELLOW='\033[0;33m'

# Check parameters
if [[ $# != 1 ]]; then
    echo "Usage: $0 ICONS8"
    echo "ICONS8 is the directory containing the Icons8 flat-color-icons repository"
    echo "(git clone https://github.com/icons8/flat-color-icons)".
    exit
fi

# Check source directory
SRCDIR=$1
if [[ ! -d $SRCDIR ]]; then
    echo -e "${RED}icons8 directory doesn't exist: ${SRCDIR}${NC}"
    exit 1
fi

# Check destination directory
DSTDIR=share/icons/database
if [[ ! -d $DSTDIR ]]; then
    echo -e "${RED}Please invoke this script from the KeePassXC source root directory.${NC}"
    exit 1
fi

# Copy one icon from the icons8 collection.
#
# Usage: copy I8NAME Cnn
# I8NAME is the file name (without extender and without
# the "icon8-" prefix) in the icons8 directory.
# Cnn is C plus the number of the database icon.
#
# Example: copy key C00
copy() {
    # The source file is:
    SRC="${SRCDIR}/svg/${1}.svg"
    if [[ ! -f $SRC ]]; then
        echo -e "${RED}Cannot find source icon for ${2} (${SRC})${NC}"
        return
    fi

    # Copy the source file to the destination, keeping
    # the source file's extension
    DST="$DSTDIR/${2}.svg"
    cp -- "$SRC" "$DST"
    echo "Copied icon for ${1} to ${DST}"
}

# Now do the actual work
#copy key                       C00_Password           # Derivative work from key
copy globe                      C01_Package_Network
copy high_priority              C02_MessageBox_Warning
copy data_protection            C03_Server             # No exact match
copy survey                     C04_Klipper
copy businessman                C05_Edu_Languages
copy services                   C06_KCMDF
#copy notepad                   C07_Kate               # Provided by paomedia/small-n-flat
copy external                   C08_Socket
copy business_contact           C09_Identity
copy address_book               C10_Kontact
copy old_time_camera            C11_Camera
copy entering_heaven_alive      C12_IRKickFlash        # No exact match
#copy keys-holder               C13_KGPG_Key3          # Derivative work from key
copy crystal_oscillator         C14_Laptop_Power
copy video_projector            C15_Scanner
copy bookmark                   C16_Mozilla_Firebird
#copy cd                        C17_CDROM_Unmount      # Provided by paomedia/small-n-flat
#copy monitor                   C18_Display            # Provided by paomedia/small-n-flat
#copy feedback                  C19_Mail_Generic       # Derivative work from feedback
copy settings                   C20_Misc
copy inspection                 C21_KOrganizer
copy file                       C22_ASCII
copy template                   C23_Icons
copy flash_on                   C24_Connect_Established
copy safe                       C25_Folder_Mail        # No exact match
#copy save                      C26_FileSave           # Provided by paomedia/small-n-flat
#copy cloud-storage             C27_NFS_Unmount        # Provided by paomedia/small-n-flat
copy film_reel                  C28_QuickTime
#copy                           C29_KGPG_Term          # Derivative work from command_line and key
#copy command_line              C30_Konsole            # Derivative work from command_line
copy print                      C31_FilePrint
copy org_unit                   C32_FSView
copy cloth                      C33_Run
copy support                    C34_Configure
#copy vpn                       C35_KRFB               # Derivative work from paomedia/small-n-flat
#copy archive-folder            C36_Ark                # Derivative work from folder
#copy percentage                C37_KPercentage        # Original work
#copy windows-client            C38_Samba_Unmount      # Derivative work from paomedia/small-n-flat
copy clock                      C39_History
copy search                     C40_Mail_Find
copy landscape                  C41_VectorGfx
copy electronics                C42_KCMMemory
copy empty_trash                C43_EditTrash
#copy                           C44_KNotes             # Provided by paomedia/small-n-flat
#copy                           C45_Cancel             # Original work
#copy                           C46_Help               # Original work
copy package                    C47_KPackage
copy folder                     C48_Folder
copy opened_folder              C49_Folder_Blue_Open
copy data_encryption            C50_Folder_Tar
#copy unlock                    C51_Decrypted          # Provided by paomedia/small-n-flat
#copy lock                      C52_Encrypted          # Provided by paomedia/small-n-flat
#copy                           C53_Apply              # Original work
#copy pencil                    C54_Signature          # Provided by paomedia/small-n-flat
copy image_file                 C55_Thumbnail
copy contacts                   C56_KAddressBook
copy data_sheet                 C57_View_Text
copy podium_with_speaker        C58_KPGP               # No exact match
#copy hammer                    C59_Package_Development # Provided by paomedia/small-n-flat
copy home                       C60_KFM_Home
#copy                           C61_Services           # Original work
copy linux                      C62_Tux
copy android_os                 C63_Feather
#copy                           C64_Apple              # Derivative work created from simple-icons apple.svg
copy wikipedia                  C65_W
#copy currency_exchange         C66_Money              # Provided by paomedia/small-n-flat
copy diploma_1                  C67_Certificate
copy smartphone_tablet          C68_Blackberry
