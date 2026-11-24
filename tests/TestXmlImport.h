/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTXMLIMPORT_H
#define KEEPASSXC_TESTXMLIMPORT_H

#include <QObject>

// round trip (XML export -> XML import) and hostile input coverage for the XML import path.
class TestXmlImport : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testRoundTrip();
    void testRoundTripKeepsEverything();
    void testAttachmentsSurviveExport();
    void testMalformedInput_data();
    void testMalformedInput();
    void testMissingFile();
    void testDuplicateGroupUuidDoesNotCreateCycle_data();
    void testDuplicateGroupUuidDoesNotCreateCycle();
    void testDeepNesting();
    void testReasonableNestingIsAccepted();
    void testEntityExpansion();
    void testMissingAttachmentContentWarns();
    void testImportedGroupKeepsNestedCustomIcons();

    // Streaming export (Database::extract to a device)
    void testExportToDeviceRoundTripsAttachments_data();
    void testExportToDeviceRoundTripsAttachments();
    void testExportToDeviceReportsWriteFailure_data();
    void testExportToDeviceReportsWriteFailure();

    // Settings of an XML file imported into an existing database (DatabaseSettingsDiff)
    void testSettingsDiffOffersEverySettingTheFileCarries();
    void testSettingsDiffIsEmptyWhenNothingDiffers();
    void testSettingsDiffShowsValuesTheWayAPersonReadsThem();
    void testSettingsApplyChangesOnlyWhatWasChosen();
    void testSettingsApplyCarriesCustomDataIconsAndDeletionRecords();
    void testSettingsApplyRecycleBinAndTemplatesFolderAfterTheTreeMoved();

    // Importing an XML file into a NEW database
    void testNewDatabaseTakesTheFilesContentAndOnlyTheChosenSettings();
    void testNewDatabaseGetsEverythingWhenNothingIsUnticked();
    void testNewDatabaseIsACompleteCopyOfTheFile();
    void testSettingsAppliedToAnOpenDatabaseAreDatedNow();

    // Text XML cannot hold (hidden characters): written as base64 with a mark, restored exactly on import
    void testHiddenCharactersSurviveXmlRoundTrip_data();
    void testHiddenCharactersSurviveXmlRoundTrip();
    void testHiddenCharactersAreWrittenAsBase64();
    void testUnpairedSurrogateIsWrittenAsUtf16Base64();
    void testCleanTextIsWrittenWithoutTheBase64Mark();
    // The same text when the database is saved as a database file and opened again
    void testHiddenCharactersSurviveSavingTheDatabaseFile_data();
    void testHiddenCharactersSurviveSavingTheDatabaseFile();
    void testPasswordsSurviveSavingTheDatabaseFile_data();
    void testPasswordsSurviveSavingTheDatabaseFile();
    void testEditingAFieldThatHeldHiddenCharactersLeavesAnOrdinaryField();
    void testNormalSaveKeepsHiddenCharactersInTitleAndPassword();
    void testCleanDataIsWrittenTheSameWithAndWithoutKeepingHiddenCharacters();
    void testSavedFileEncryptsEntryFieldsThatHoldHiddenCharacters();
    void testProtectedValueEncoding_data();
    void testProtectedValueEncoding();

    // The file that lists, next to an XML export, what was stored as base64
    void testHiddenCharacterListFileName_data();
    void testHiddenCharacterListFileName();
    void testHiddenCharacterListFile();
    void testOlderHiddenCharacterListIsRemovedOnlyWhenItIsOurs();
    void testExportListsWhereHiddenCharactersWereStoredAsBase64();
    void testExportListsNothingWhenThereAreNoHiddenCharacters();
};

#endif // KEEPASSXC_TESTXMLIMPORT_H
