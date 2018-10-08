/*
 *  Copyright (C) 2018 Kyle Kneitinger <kyle@kneit.in>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_CONFIGDEPRECATIONS_H
#define KEEPASSX_CONFIGDEPRECATIONS_H

/*
 * Map of configuration file settings that are either deprecated, or have
 * had their name changed.  Entries in the map are of the form
 *     {oldName, newName}
 * Set newName to "REMOVED" to remove the setting from the file.
 */
static const QMap<QString, QString> deprecationMap = {
    // >2.3.4
    {"security/hidepassworddetails", "security/HidePasswordPreviewPanel"},
    // >2.3.4
    {"GUI/HideDetailsView", "GUI/HidePreviewPanel"},
    // >2.3.4
    {"GUI/DetailSplitterState", "GUI/PreviewSplitterState"},
    // >2.3.4
    {"security/IconDownloadFallbackToGoogle", "security/IconDownloadFallback"},
};

#endif // KEEPASSX_CONFIGDEPRECATIONS_H
