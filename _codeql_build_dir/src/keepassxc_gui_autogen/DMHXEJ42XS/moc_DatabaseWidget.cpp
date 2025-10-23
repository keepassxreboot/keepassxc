/****************************************************************************
** Meta object code from reading C++ file 'DatabaseWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/DatabaseWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatabaseWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DatabaseWidget_t {
    QByteArrayData data[182];
    char stringdata0[2838];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DatabaseWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DatabaseWidget_t qt_meta_stringdata_DatabaseWidget = {
    {
QT_MOC_LITERAL(0, 0, 14), // "DatabaseWidget"
QT_MOC_LITERAL(1, 15, 23), // "databaseFilePathChanged"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 7), // "oldPath"
QT_MOC_LITERAL(4, 48, 7), // "newPath"
QT_MOC_LITERAL(5, 56, 16), // "databaseModified"
QT_MOC_LITERAL(6, 73, 22), // "databaseNonDataChanged"
QT_MOC_LITERAL(7, 96, 13), // "databaseSaved"
QT_MOC_LITERAL(8, 110, 21), // "databaseAboutToUnlock"
QT_MOC_LITERAL(9, 132, 16), // "databaseUnlocked"
QT_MOC_LITERAL(10, 149, 21), // "databaseLockRequested"
QT_MOC_LITERAL(11, 171, 14), // "databaseLocked"
QT_MOC_LITERAL(12, 186, 16), // "databaseReplaced"
QT_MOC_LITERAL(13, 203, 24), // "QSharedPointer<Database>"
QT_MOC_LITERAL(14, 228, 5), // "oldDb"
QT_MOC_LITERAL(15, 234, 5), // "newDb"
QT_MOC_LITERAL(16, 240, 12), // "closeRequest"
QT_MOC_LITERAL(17, 253, 18), // "currentModeChanged"
QT_MOC_LITERAL(18, 272, 20), // "DatabaseWidget::Mode"
QT_MOC_LITERAL(19, 293, 4), // "mode"
QT_MOC_LITERAL(20, 298, 12), // "groupChanged"
QT_MOC_LITERAL(21, 311, 21), // "entrySelectionChanged"
QT_MOC_LITERAL(22, 333, 19), // "requestOpenDatabase"
QT_MOC_LITERAL(23, 353, 8), // "filePath"
QT_MOC_LITERAL(24, 362, 12), // "inBackground"
QT_MOC_LITERAL(25, 375, 8), // "password"
QT_MOC_LITERAL(26, 384, 7), // "keyFile"
QT_MOC_LITERAL(27, 392, 14), // "databaseMerged"
QT_MOC_LITERAL(28, 407, 8), // "mergedDb"
QT_MOC_LITERAL(29, 416, 22), // "databaseSyncInProgress"
QT_MOC_LITERAL(30, 439, 21), // "databaseSyncCompleted"
QT_MOC_LITERAL(31, 461, 8), // "syncName"
QT_MOC_LITERAL(32, 470, 18), // "databaseSyncFailed"
QT_MOC_LITERAL(33, 489, 5), // "error"
QT_MOC_LITERAL(34, 495, 24), // "databaseSyncUnlockFailed"
QT_MOC_LITERAL(35, 520, 27), // "RemoteHandler::RemoteResult"
QT_MOC_LITERAL(36, 548, 6), // "result"
QT_MOC_LITERAL(37, 555, 20), // "databaseSyncUnlocked"
QT_MOC_LITERAL(38, 576, 29), // "unlockDatabaseInDialogForSync"
QT_MOC_LITERAL(39, 606, 18), // "updateSyncProgress"
QT_MOC_LITERAL(40, 625, 10), // "percentage"
QT_MOC_LITERAL(41, 636, 7), // "message"
QT_MOC_LITERAL(42, 644, 25), // "groupContextMenuRequested"
QT_MOC_LITERAL(43, 670, 9), // "globalPos"
QT_MOC_LITERAL(44, 680, 25), // "entryContextMenuRequested"
QT_MOC_LITERAL(45, 706, 23), // "listModeAboutToActivate"
QT_MOC_LITERAL(46, 730, 17), // "listModeActivated"
QT_MOC_LITERAL(47, 748, 25), // "searchModeAboutToActivate"
QT_MOC_LITERAL(48, 774, 19), // "searchModeActivated"
QT_MOC_LITERAL(49, 794, 20), // "splitterSizesChanged"
QT_MOC_LITERAL(50, 815, 21), // "entryViewStateChanged"
QT_MOC_LITERAL(51, 837, 11), // "clearSearch"
QT_MOC_LITERAL(52, 849, 21), // "requestGlobalAutoType"
QT_MOC_LITERAL(53, 871, 6), // "search"
QT_MOC_LITERAL(54, 878, 13), // "requestSearch"
QT_MOC_LITERAL(55, 892, 11), // "reloadBegin"
QT_MOC_LITERAL(56, 904, 9), // "reloadEnd"
QT_MOC_LITERAL(57, 914, 4), // "lock"
QT_MOC_LITERAL(58, 919, 4), // "save"
QT_MOC_LITERAL(59, 924, 6), // "saveAs"
QT_MOC_LITERAL(60, 931, 10), // "saveBackup"
QT_MOC_LITERAL(61, 942, 15), // "replaceDatabase"
QT_MOC_LITERAL(62, 958, 2), // "db"
QT_MOC_LITERAL(63, 961, 11), // "createEntry"
QT_MOC_LITERAL(64, 973, 10), // "cloneEntry"
QT_MOC_LITERAL(65, 984, 21), // "expireSelectedEntries"
QT_MOC_LITERAL(66, 1006, 21), // "deleteSelectedEntries"
QT_MOC_LITERAL(67, 1028, 22), // "restoreSelectedEntries"
QT_MOC_LITERAL(68, 1051, 13), // "deleteEntries"
QT_MOC_LITERAL(69, 1065, 13), // "QList<Entry*>"
QT_MOC_LITERAL(70, 1079, 7), // "entries"
QT_MOC_LITERAL(71, 1087, 7), // "confirm"
QT_MOC_LITERAL(72, 1095, 14), // "focusOnEntries"
QT_MOC_LITERAL(73, 1110, 13), // "editIfFocused"
QT_MOC_LITERAL(74, 1124, 13), // "focusOnGroups"
QT_MOC_LITERAL(75, 1138, 11), // "moveEntryUp"
QT_MOC_LITERAL(76, 1150, 13), // "moveEntryDown"
QT_MOC_LITERAL(77, 1164, 9), // "copyTitle"
QT_MOC_LITERAL(78, 1174, 12), // "copyUsername"
QT_MOC_LITERAL(79, 1187, 12), // "copyPassword"
QT_MOC_LITERAL(80, 1200, 7), // "copyURL"
QT_MOC_LITERAL(81, 1208, 9), // "copyNotes"
QT_MOC_LITERAL(82, 1218, 13), // "copyAttribute"
QT_MOC_LITERAL(83, 1232, 8), // "QAction*"
QT_MOC_LITERAL(84, 1241, 6), // "action"
QT_MOC_LITERAL(85, 1248, 24), // "copyFocusedTextSelection"
QT_MOC_LITERAL(86, 1273, 11), // "filterByTag"
QT_MOC_LITERAL(87, 1285, 6), // "setTag"
QT_MOC_LITERAL(88, 1292, 8), // "showTotp"
QT_MOC_LITERAL(89, 1301, 17), // "showTotpKeyQrCode"
QT_MOC_LITERAL(90, 1319, 8), // "copyTotp"
QT_MOC_LITERAL(91, 1328, 16), // "copyPasswordTotp"
QT_MOC_LITERAL(92, 1345, 9), // "setupTotp"
QT_MOC_LITERAL(93, 1355, 15), // "performAutoType"
QT_MOC_LITERAL(94, 1371, 8), // "sequence"
QT_MOC_LITERAL(95, 1380, 23), // "performAutoTypeUsername"
QT_MOC_LITERAL(96, 1404, 28), // "performAutoTypeUsernameEnter"
QT_MOC_LITERAL(97, 1433, 23), // "performAutoTypePassword"
QT_MOC_LITERAL(98, 1457, 28), // "performAutoTypePasswordEnter"
QT_MOC_LITERAL(99, 1486, 19), // "performAutoTypeTOTP"
QT_MOC_LITERAL(100, 1506, 18), // "performAutoTypeURL"
QT_MOC_LITERAL(101, 1525, 23), // "performAutoTypeURLEnter"
QT_MOC_LITERAL(102, 1549, 27), // "setClipboardTextAndMinimize"
QT_MOC_LITERAL(103, 1577, 4), // "text"
QT_MOC_LITERAL(104, 1582, 7), // "openUrl"
QT_MOC_LITERAL(105, 1590, 24), // "downloadSelectedFavicons"
QT_MOC_LITERAL(106, 1615, 19), // "downloadAllFavicons"
QT_MOC_LITERAL(107, 1635, 27), // "downloadFaviconInBackground"
QT_MOC_LITERAL(108, 1663, 6), // "Entry*"
QT_MOC_LITERAL(109, 1670, 5), // "entry"
QT_MOC_LITERAL(110, 1676, 15), // "openUrlForEntry"
QT_MOC_LITERAL(111, 1692, 11), // "createGroup"
QT_MOC_LITERAL(112, 1704, 10), // "cloneGroup"
QT_MOC_LITERAL(113, 1715, 11), // "deleteGroup"
QT_MOC_LITERAL(114, 1727, 16), // "switchToMainView"
QT_MOC_LITERAL(115, 1744, 22), // "previousDialogAccepted"
QT_MOC_LITERAL(116, 1767, 17), // "switchToEntryEdit"
QT_MOC_LITERAL(117, 1785, 17), // "switchToGroupEdit"
QT_MOC_LITERAL(118, 1803, 13), // "sortGroupsAsc"
QT_MOC_LITERAL(119, 1817, 14), // "sortGroupsDesc"
QT_MOC_LITERAL(120, 1832, 24), // "switchToDatabaseSecurity"
QT_MOC_LITERAL(121, 1857, 23), // "switchToDatabaseReports"
QT_MOC_LITERAL(122, 1881, 24), // "switchToDatabaseSettings"
QT_MOC_LITERAL(123, 1906, 22), // "switchToRemoteSettings"
QT_MOC_LITERAL(124, 1929, 20), // "switchToOpenDatabase"
QT_MOC_LITERAL(125, 1950, 21), // "performUnlockDatabase"
QT_MOC_LITERAL(126, 1972, 7), // "keyfile"
QT_MOC_LITERAL(127, 1980, 15), // "emptyRecycleBin"
QT_MOC_LITERAL(128, 1996, 10), // "searchtext"
QT_MOC_LITERAL(129, 2007, 10), // "saveSearch"
QT_MOC_LITERAL(130, 2018, 12), // "deleteSearch"
QT_MOC_LITERAL(131, 2031, 4), // "name"
QT_MOC_LITERAL(132, 2036, 22), // "setSearchCaseSensitive"
QT_MOC_LITERAL(133, 2059, 5), // "state"
QT_MOC_LITERAL(134, 2065, 19), // "setSearchLimitGroup"
QT_MOC_LITERAL(135, 2085, 9), // "endSearch"
QT_MOC_LITERAL(136, 2095, 11), // "showMessage"
QT_MOC_LITERAL(137, 2107, 26), // "MessageWidget::MessageType"
QT_MOC_LITERAL(138, 2134, 4), // "type"
QT_MOC_LITERAL(139, 2139, 15), // "showClosebutton"
QT_MOC_LITERAL(140, 2155, 15), // "autoHideTimeout"
QT_MOC_LITERAL(141, 2171, 16), // "showErrorMessage"
QT_MOC_LITERAL(142, 2188, 12), // "errorMessage"
QT_MOC_LITERAL(143, 2201, 11), // "hideMessage"
QT_MOC_LITERAL(144, 2213, 20), // "triggerAutosaveTimer"
QT_MOC_LITERAL(145, 2234, 29), // "entryActivationSignalReceived"
QT_MOC_LITERAL(146, 2264, 23), // "EntryModel::ModelColumn"
QT_MOC_LITERAL(147, 2288, 6), // "column"
QT_MOC_LITERAL(148, 2295, 21), // "switchBackToEntryEdit"
QT_MOC_LITERAL(149, 2317, 19), // "switchToHistoryView"
QT_MOC_LITERAL(150, 2337, 6), // "create"
QT_MOC_LITERAL(151, 2344, 6), // "Group*"
QT_MOC_LITERAL(152, 2351, 29), // "emitGroupContextMenuRequested"
QT_MOC_LITERAL(153, 2381, 3), // "pos"
QT_MOC_LITERAL(154, 2385, 29), // "emitEntryContextMenuRequested"
QT_MOC_LITERAL(155, 2415, 14), // "onEntryChanged"
QT_MOC_LITERAL(156, 2430, 14), // "onGroupChanged"
QT_MOC_LITERAL(157, 2445, 18), // "onDatabaseModified"
QT_MOC_LITERAL(158, 2464, 24), // "onDatabaseNonDataChanged"
QT_MOC_LITERAL(159, 2489, 22), // "onAutosaveDelayTimeout"
QT_MOC_LITERAL(160, 2512, 22), // "connectDatabaseSignals"
QT_MOC_LITERAL(161, 2535, 12), // "loadDatabase"
QT_MOC_LITERAL(162, 2548, 8), // "accepted"
QT_MOC_LITERAL(163, 2557, 14), // "unlockDatabase"
QT_MOC_LITERAL(164, 2572, 13), // "mergeDatabase"
QT_MOC_LITERAL(165, 2586, 20), // "syncUnlockedDatabase"
QT_MOC_LITERAL(166, 2607, 16), // "syncWithDatabase"
QT_MOC_LITERAL(167, 2624, 7), // "otherDb"
QT_MOC_LITERAL(168, 2632, 8), // "QString&"
QT_MOC_LITERAL(169, 2641, 19), // "uploadAndFinishSync"
QT_MOC_LITERAL(170, 2661, 19), // "const RemoteParams*"
QT_MOC_LITERAL(171, 2681, 6), // "params"
QT_MOC_LITERAL(172, 2688, 10), // "finishSync"
QT_MOC_LITERAL(173, 2699, 22), // "emitCurrentModeChanged"
QT_MOC_LITERAL(174, 2722, 18), // "reloadDatabaseFile"
QT_MOC_LITERAL(175, 2741, 15), // "triggeredBySave"
QT_MOC_LITERAL(176, 2757, 22), // "restoreGroupEntryFocus"
QT_MOC_LITERAL(177, 2780, 9), // "groupUuid"
QT_MOC_LITERAL(178, 2790, 9), // "EntryUuid"
QT_MOC_LITERAL(179, 2800, 15), // "onConfigChanged"
QT_MOC_LITERAL(180, 2816, 17), // "Config::ConfigKey"
QT_MOC_LITERAL(181, 2834, 3) // "key"

    },
    "DatabaseWidget\0databaseFilePathChanged\0"
    "\0oldPath\0newPath\0databaseModified\0"
    "databaseNonDataChanged\0databaseSaved\0"
    "databaseAboutToUnlock\0databaseUnlocked\0"
    "databaseLockRequested\0databaseLocked\0"
    "databaseReplaced\0QSharedPointer<Database>\0"
    "oldDb\0newDb\0closeRequest\0currentModeChanged\0"
    "DatabaseWidget::Mode\0mode\0groupChanged\0"
    "entrySelectionChanged\0requestOpenDatabase\0"
    "filePath\0inBackground\0password\0keyFile\0"
    "databaseMerged\0mergedDb\0databaseSyncInProgress\0"
    "databaseSyncCompleted\0syncName\0"
    "databaseSyncFailed\0error\0"
    "databaseSyncUnlockFailed\0"
    "RemoteHandler::RemoteResult\0result\0"
    "databaseSyncUnlocked\0unlockDatabaseInDialogForSync\0"
    "updateSyncProgress\0percentage\0message\0"
    "groupContextMenuRequested\0globalPos\0"
    "entryContextMenuRequested\0"
    "listModeAboutToActivate\0listModeActivated\0"
    "searchModeAboutToActivate\0searchModeActivated\0"
    "splitterSizesChanged\0entryViewStateChanged\0"
    "clearSearch\0requestGlobalAutoType\0"
    "search\0requestSearch\0reloadBegin\0"
    "reloadEnd\0lock\0save\0saveAs\0saveBackup\0"
    "replaceDatabase\0db\0createEntry\0"
    "cloneEntry\0expireSelectedEntries\0"
    "deleteSelectedEntries\0restoreSelectedEntries\0"
    "deleteEntries\0QList<Entry*>\0entries\0"
    "confirm\0focusOnEntries\0editIfFocused\0"
    "focusOnGroups\0moveEntryUp\0moveEntryDown\0"
    "copyTitle\0copyUsername\0copyPassword\0"
    "copyURL\0copyNotes\0copyAttribute\0"
    "QAction*\0action\0copyFocusedTextSelection\0"
    "filterByTag\0setTag\0showTotp\0"
    "showTotpKeyQrCode\0copyTotp\0copyPasswordTotp\0"
    "setupTotp\0performAutoType\0sequence\0"
    "performAutoTypeUsername\0"
    "performAutoTypeUsernameEnter\0"
    "performAutoTypePassword\0"
    "performAutoTypePasswordEnter\0"
    "performAutoTypeTOTP\0performAutoTypeURL\0"
    "performAutoTypeURLEnter\0"
    "setClipboardTextAndMinimize\0text\0"
    "openUrl\0downloadSelectedFavicons\0"
    "downloadAllFavicons\0downloadFaviconInBackground\0"
    "Entry*\0entry\0openUrlForEntry\0createGroup\0"
    "cloneGroup\0deleteGroup\0switchToMainView\0"
    "previousDialogAccepted\0switchToEntryEdit\0"
    "switchToGroupEdit\0sortGroupsAsc\0"
    "sortGroupsDesc\0switchToDatabaseSecurity\0"
    "switchToDatabaseReports\0"
    "switchToDatabaseSettings\0"
    "switchToRemoteSettings\0switchToOpenDatabase\0"
    "performUnlockDatabase\0keyfile\0"
    "emptyRecycleBin\0searchtext\0saveSearch\0"
    "deleteSearch\0name\0setSearchCaseSensitive\0"
    "state\0setSearchLimitGroup\0endSearch\0"
    "showMessage\0MessageWidget::MessageType\0"
    "type\0showClosebutton\0autoHideTimeout\0"
    "showErrorMessage\0errorMessage\0hideMessage\0"
    "triggerAutosaveTimer\0entryActivationSignalReceived\0"
    "EntryModel::ModelColumn\0column\0"
    "switchBackToEntryEdit\0switchToHistoryView\0"
    "create\0Group*\0emitGroupContextMenuRequested\0"
    "pos\0emitEntryContextMenuRequested\0"
    "onEntryChanged\0onGroupChanged\0"
    "onDatabaseModified\0onDatabaseNonDataChanged\0"
    "onAutosaveDelayTimeout\0connectDatabaseSignals\0"
    "loadDatabase\0accepted\0unlockDatabase\0"
    "mergeDatabase\0syncUnlockedDatabase\0"
    "syncWithDatabase\0otherDb\0QString&\0"
    "uploadAndFinishSync\0const RemoteParams*\0"
    "params\0finishSync\0emitCurrentModeChanged\0"
    "reloadDatabaseFile\0triggeredBySave\0"
    "restoreGroupEntryFocus\0groupUuid\0"
    "EntryUuid\0onConfigChanged\0Config::ConfigKey\0"
    "key"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DatabaseWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
     138,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      35,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,  704,    2, 0x06 /* Public */,
       5,    0,  709,    2, 0x06 /* Public */,
       6,    0,  710,    2, 0x06 /* Public */,
       7,    0,  711,    2, 0x06 /* Public */,
       8,    0,  712,    2, 0x06 /* Public */,
       9,    0,  713,    2, 0x06 /* Public */,
      10,    0,  714,    2, 0x06 /* Public */,
      11,    0,  715,    2, 0x06 /* Public */,
      12,    2,  716,    2, 0x06 /* Public */,
      16,    0,  721,    2, 0x06 /* Public */,
      17,    1,  722,    2, 0x06 /* Public */,
      20,    0,  725,    2, 0x06 /* Public */,
      21,    0,  726,    2, 0x06 /* Public */,
      22,    4,  727,    2, 0x06 /* Public */,
      27,    1,  736,    2, 0x06 /* Public */,
      29,    0,  739,    2, 0x06 /* Public */,
      30,    1,  740,    2, 0x06 /* Public */,
      32,    2,  743,    2, 0x06 /* Public */,
      34,    1,  748,    2, 0x06 /* Public */,
      37,    1,  751,    2, 0x06 /* Public */,
      38,    1,  754,    2, 0x06 /* Public */,
      39,    2,  757,    2, 0x06 /* Public */,
      42,    1,  762,    2, 0x06 /* Public */,
      44,    1,  765,    2, 0x06 /* Public */,
      45,    0,  768,    2, 0x06 /* Public */,
      46,    0,  769,    2, 0x06 /* Public */,
      47,    0,  770,    2, 0x06 /* Public */,
      48,    0,  771,    2, 0x06 /* Public */,
      49,    0,  772,    2, 0x06 /* Public */,
      50,    0,  773,    2, 0x06 /* Public */,
      51,    0,  774,    2, 0x06 /* Public */,
      52,    1,  775,    2, 0x06 /* Public */,
      54,    1,  778,    2, 0x06 /* Public */,
      55,    0,  781,    2, 0x06 /* Public */,
      56,    0,  782,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      57,    0,  783,    2, 0x0a /* Public */,
      58,    0,  784,    2, 0x0a /* Public */,
      59,    0,  785,    2, 0x0a /* Public */,
      60,    0,  786,    2, 0x0a /* Public */,
      61,    1,  787,    2, 0x0a /* Public */,
      63,    0,  790,    2, 0x0a /* Public */,
      64,    0,  791,    2, 0x0a /* Public */,
      65,    0,  792,    2, 0x0a /* Public */,
      66,    0,  793,    2, 0x0a /* Public */,
      67,    0,  794,    2, 0x0a /* Public */,
      68,    2,  795,    2, 0x0a /* Public */,
      68,    1,  800,    2, 0x2a /* Public | MethodCloned */,
      72,    1,  803,    2, 0x0a /* Public */,
      72,    0,  806,    2, 0x2a /* Public | MethodCloned */,
      74,    1,  807,    2, 0x0a /* Public */,
      74,    0,  810,    2, 0x2a /* Public | MethodCloned */,
      75,    0,  811,    2, 0x0a /* Public */,
      76,    0,  812,    2, 0x0a /* Public */,
      77,    0,  813,    2, 0x0a /* Public */,
      78,    0,  814,    2, 0x0a /* Public */,
      79,    0,  815,    2, 0x0a /* Public */,
      80,    0,  816,    2, 0x0a /* Public */,
      81,    0,  817,    2, 0x0a /* Public */,
      82,    1,  818,    2, 0x0a /* Public */,
      85,    0,  821,    2, 0x0a /* Public */,
      86,    0,  822,    2, 0x0a /* Public */,
      87,    1,  823,    2, 0x0a /* Public */,
      88,    0,  826,    2, 0x0a /* Public */,
      89,    0,  827,    2, 0x0a /* Public */,
      90,    0,  828,    2, 0x0a /* Public */,
      91,    0,  829,    2, 0x0a /* Public */,
      92,    0,  830,    2, 0x0a /* Public */,
      93,    1,  831,    2, 0x0a /* Public */,
      93,    0,  834,    2, 0x2a /* Public | MethodCloned */,
      95,    0,  835,    2, 0x0a /* Public */,
      96,    0,  836,    2, 0x0a /* Public */,
      97,    0,  837,    2, 0x0a /* Public */,
      98,    0,  838,    2, 0x0a /* Public */,
      99,    0,  839,    2, 0x0a /* Public */,
     100,    0,  840,    2, 0x0a /* Public */,
     101,    0,  841,    2, 0x0a /* Public */,
     102,    1,  842,    2, 0x0a /* Public */,
     104,    0,  845,    2, 0x0a /* Public */,
     105,    0,  846,    2, 0x0a /* Public */,
     106,    0,  847,    2, 0x0a /* Public */,
     107,    1,  848,    2, 0x0a /* Public */,
     110,    1,  851,    2, 0x0a /* Public */,
     111,    0,  854,    2, 0x0a /* Public */,
     112,    0,  855,    2, 0x0a /* Public */,
     113,    0,  856,    2, 0x0a /* Public */,
     114,    1,  857,    2, 0x0a /* Public */,
     114,    0,  860,    2, 0x2a /* Public | MethodCloned */,
     116,    0,  861,    2, 0x0a /* Public */,
     117,    0,  862,    2, 0x0a /* Public */,
     118,    0,  863,    2, 0x0a /* Public */,
     119,    0,  864,    2, 0x0a /* Public */,
     120,    0,  865,    2, 0x0a /* Public */,
     121,    0,  866,    2, 0x0a /* Public */,
     122,    0,  867,    2, 0x0a /* Public */,
     123,    0,  868,    2, 0x0a /* Public */,
     124,    0,  869,    2, 0x0a /* Public */,
     124,    1,  870,    2, 0x0a /* Public */,
     124,    3,  873,    2, 0x0a /* Public */,
     125,    2,  880,    2, 0x0a /* Public */,
     125,    1,  885,    2, 0x2a /* Public | MethodCloned */,
     127,    0,  888,    2, 0x0a /* Public */,
      53,    1,  889,    2, 0x0a /* Public */,
     129,    1,  892,    2, 0x0a /* Public */,
     130,    1,  895,    2, 0x0a /* Public */,
     132,    1,  898,    2, 0x0a /* Public */,
     134,    1,  901,    2, 0x0a /* Public */,
     135,    0,  904,    2, 0x0a /* Public */,
     136,    4,  905,    2, 0x0a /* Public */,
     136,    3,  914,    2, 0x2a /* Public | MethodCloned */,
     136,    2,  921,    2, 0x2a /* Public | MethodCloned */,
     141,    1,  926,    2, 0x0a /* Public */,
     143,    0,  929,    2, 0x0a /* Public */,
     144,    0,  930,    2, 0x0a /* Public */,
     145,    2,  931,    2, 0x08 /* Private */,
     148,    0,  936,    2, 0x08 /* Private */,
     149,    1,  937,    2, 0x08 /* Private */,
     116,    1,  940,    2, 0x08 /* Private */,
     116,    2,  943,    2, 0x08 /* Private */,
     117,    2,  948,    2, 0x08 /* Private */,
     152,    1,  953,    2, 0x08 /* Private */,
     154,    1,  956,    2, 0x08 /* Private */,
     155,    1,  959,    2, 0x08 /* Private */,
     156,    0,  962,    2, 0x08 /* Private */,
     157,    0,  963,    2, 0x08 /* Private */,
     158,    0,  964,    2, 0x08 /* Private */,
     159,    0,  965,    2, 0x08 /* Private */,
     160,    0,  966,    2, 0x08 /* Private */,
     161,    1,  967,    2, 0x08 /* Private */,
     163,    1,  970,    2, 0x08 /* Private */,
     164,    1,  973,    2, 0x08 /* Private */,
     165,    1,  976,    2, 0x08 /* Private */,
     166,    2,  979,    2, 0x08 /* Private */,
     169,    2,  984,    2, 0x08 /* Private */,
     172,    2,  989,    2, 0x08 /* Private */,
     173,    0,  994,    2, 0x08 /* Private */,
     174,    1,  995,    2, 0x08 /* Private */,
     176,    2,  998,    2, 0x08 /* Private */,
     179,    1, 1003,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 13, 0x80000000 | 13,   14,   15,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 18,   19,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool, QMetaType::QString, QMetaType::QString,   23,   24,   25,   26,
    QMetaType::Void, 0x80000000 | 13,   28,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   31,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   31,   33,
    QMetaType::Void, 0x80000000 | 35,   36,
    QMetaType::Void, 0x80000000 | 35,   36,
    QMetaType::Void, QMetaType::QString,   23,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   40,   41,
    QMetaType::Void, QMetaType::QPoint,   43,
    QMetaType::Void, QMetaType::QPoint,   43,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   53,
    QMetaType::Void, QMetaType::QString,   53,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void, 0x80000000 | 13,   62,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 69, QMetaType::Bool,   70,   71,
    QMetaType::Void, 0x80000000 | 69,   70,
    QMetaType::Void, QMetaType::Bool,   73,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   73,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 83,   84,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 83,   84,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   94,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  103,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 108,  109,
    QMetaType::Void, 0x80000000 | 108,  109,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,  115,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   23,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   23,   25,   26,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   25,  126,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,  128,
    QMetaType::Void, QMetaType::QString,  128,
    QMetaType::Void, QMetaType::QString,  131,
    QMetaType::Void, QMetaType::Bool,  133,
    QMetaType::Void, QMetaType::Bool,  133,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 137, QMetaType::Bool, QMetaType::Int,  103,  138,  139,  140,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 137, QMetaType::Bool,  103,  138,  139,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 137,  103,  138,
    QMetaType::Void, QMetaType::QString,  142,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 108, 0x80000000 | 146,  109,  147,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 108,  109,
    QMetaType::Void, 0x80000000 | 108,    2,
    QMetaType::Void, 0x80000000 | 108, QMetaType::Bool,  109,  150,
    QMetaType::Void, 0x80000000 | 151, QMetaType::Bool,  109,  150,
    QMetaType::Void, QMetaType::QPoint,  153,
    QMetaType::Void, QMetaType::QPoint,  153,
    QMetaType::Void, 0x80000000 | 108,  109,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,  162,
    QMetaType::Void, QMetaType::Bool,  162,
    QMetaType::Void, QMetaType::Bool,  162,
    QMetaType::Void, QMetaType::Bool,  162,
    QMetaType::Bool, 0x80000000 | 13, 0x80000000 | 168,  167,   33,
    QMetaType::Void, 0x80000000 | 170, 0x80000000 | 35,  171,   36,
    QMetaType::Void, 0x80000000 | 170, 0x80000000 | 35,  171,   36,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,  175,
    QMetaType::Void, QMetaType::QUuid, QMetaType::QUuid,  177,  178,
    QMetaType::Void, 0x80000000 | 180,  181,

       0        // eod
};

void DatabaseWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DatabaseWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->databaseFilePathChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->databaseModified(); break;
        case 2: _t->databaseNonDataChanged(); break;
        case 3: _t->databaseSaved(); break;
        case 4: _t->databaseAboutToUnlock(); break;
        case 5: _t->databaseUnlocked(); break;
        case 6: _t->databaseLockRequested(); break;
        case 7: _t->databaseLocked(); break;
        case 8: _t->databaseReplaced((*reinterpret_cast< const QSharedPointer<Database>(*)>(_a[1])),(*reinterpret_cast< const QSharedPointer<Database>(*)>(_a[2]))); break;
        case 9: _t->closeRequest(); break;
        case 10: _t->currentModeChanged((*reinterpret_cast< DatabaseWidget::Mode(*)>(_a[1]))); break;
        case 11: _t->groupChanged(); break;
        case 12: _t->entrySelectionChanged(); break;
        case 13: _t->requestOpenDatabase((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4]))); break;
        case 14: _t->databaseMerged((*reinterpret_cast< QSharedPointer<Database>(*)>(_a[1]))); break;
        case 15: _t->databaseSyncInProgress(); break;
        case 16: _t->databaseSyncCompleted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->databaseSyncFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 18: _t->databaseSyncUnlockFailed((*reinterpret_cast< const RemoteHandler::RemoteResult(*)>(_a[1]))); break;
        case 19: _t->databaseSyncUnlocked((*reinterpret_cast< const RemoteHandler::RemoteResult(*)>(_a[1]))); break;
        case 20: _t->unlockDatabaseInDialogForSync((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 21: _t->updateSyncProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 22: _t->groupContextMenuRequested((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 23: _t->entryContextMenuRequested((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 24: _t->listModeAboutToActivate(); break;
        case 25: _t->listModeActivated(); break;
        case 26: _t->searchModeAboutToActivate(); break;
        case 27: _t->searchModeActivated(); break;
        case 28: _t->splitterSizesChanged(); break;
        case 29: _t->entryViewStateChanged(); break;
        case 30: _t->clearSearch(); break;
        case 31: _t->requestGlobalAutoType((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 32: _t->requestSearch((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 33: _t->reloadBegin(); break;
        case 34: _t->reloadEnd(); break;
        case 35: { bool _r = _t->lock();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 36: { bool _r = _t->save();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 37: { bool _r = _t->saveAs();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 38: { bool _r = _t->saveBackup();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 39: _t->replaceDatabase((*reinterpret_cast< QSharedPointer<Database>(*)>(_a[1]))); break;
        case 40: _t->createEntry(); break;
        case 41: _t->cloneEntry(); break;
        case 42: _t->expireSelectedEntries(); break;
        case 43: _t->deleteSelectedEntries(); break;
        case 44: _t->restoreSelectedEntries(); break;
        case 45: _t->deleteEntries((*reinterpret_cast< QList<Entry*>(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 46: _t->deleteEntries((*reinterpret_cast< QList<Entry*>(*)>(_a[1]))); break;
        case 47: _t->focusOnEntries((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 48: _t->focusOnEntries(); break;
        case 49: _t->focusOnGroups((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 50: _t->focusOnGroups(); break;
        case 51: _t->moveEntryUp(); break;
        case 52: _t->moveEntryDown(); break;
        case 53: _t->copyTitle(); break;
        case 54: _t->copyUsername(); break;
        case 55: _t->copyPassword(); break;
        case 56: _t->copyURL(); break;
        case 57: _t->copyNotes(); break;
        case 58: _t->copyAttribute((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 59: { bool _r = _t->copyFocusedTextSelection();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 60: _t->filterByTag(); break;
        case 61: _t->setTag((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 62: _t->showTotp(); break;
        case 63: _t->showTotpKeyQrCode(); break;
        case 64: _t->copyTotp(); break;
        case 65: _t->copyPasswordTotp(); break;
        case 66: _t->setupTotp(); break;
        case 67: _t->performAutoType((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 68: _t->performAutoType(); break;
        case 69: _t->performAutoTypeUsername(); break;
        case 70: _t->performAutoTypeUsernameEnter(); break;
        case 71: _t->performAutoTypePassword(); break;
        case 72: _t->performAutoTypePasswordEnter(); break;
        case 73: _t->performAutoTypeTOTP(); break;
        case 74: _t->performAutoTypeURL(); break;
        case 75: _t->performAutoTypeURLEnter(); break;
        case 76: _t->setClipboardTextAndMinimize((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 77: _t->openUrl(); break;
        case 78: _t->downloadSelectedFavicons(); break;
        case 79: _t->downloadAllFavicons(); break;
        case 80: _t->downloadFaviconInBackground((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 81: _t->openUrlForEntry((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 82: _t->createGroup(); break;
        case 83: _t->cloneGroup(); break;
        case 84: _t->deleteGroup(); break;
        case 85: _t->switchToMainView((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 86: _t->switchToMainView(); break;
        case 87: _t->switchToEntryEdit(); break;
        case 88: _t->switchToGroupEdit(); break;
        case 89: _t->sortGroupsAsc(); break;
        case 90: _t->sortGroupsDesc(); break;
        case 91: _t->switchToDatabaseSecurity(); break;
        case 92: _t->switchToDatabaseReports(); break;
        case 93: _t->switchToDatabaseSettings(); break;
        case 94: _t->switchToRemoteSettings(); break;
        case 95: _t->switchToOpenDatabase(); break;
        case 96: _t->switchToOpenDatabase((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 97: _t->switchToOpenDatabase((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 98: _t->performUnlockDatabase((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 99: _t->performUnlockDatabase((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 100: _t->emptyRecycleBin(); break;
        case 101: _t->search((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 102: _t->saveSearch((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 103: _t->deleteSearch((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 104: _t->setSearchCaseSensitive((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 105: _t->setSearchLimitGroup((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 106: _t->endSearch(); break;
        case 107: _t->showMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 108: _t->showMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 109: _t->showMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2]))); break;
        case 110: _t->showErrorMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 111: _t->hideMessage(); break;
        case 112: _t->triggerAutosaveTimer(); break;
        case 113: _t->entryActivationSignalReceived((*reinterpret_cast< Entry*(*)>(_a[1])),(*reinterpret_cast< EntryModel::ModelColumn(*)>(_a[2]))); break;
        case 114: _t->switchBackToEntryEdit(); break;
        case 115: _t->switchToHistoryView((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 116: _t->switchToEntryEdit((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 117: _t->switchToEntryEdit((*reinterpret_cast< Entry*(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 118: _t->switchToGroupEdit((*reinterpret_cast< Group*(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 119: _t->emitGroupContextMenuRequested((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 120: _t->emitEntryContextMenuRequested((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 121: _t->onEntryChanged((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 122: _t->onGroupChanged(); break;
        case 123: _t->onDatabaseModified(); break;
        case 124: _t->onDatabaseNonDataChanged(); break;
        case 125: _t->onAutosaveDelayTimeout(); break;
        case 126: _t->connectDatabaseSignals(); break;
        case 127: _t->loadDatabase((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 128: _t->unlockDatabase((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 129: _t->mergeDatabase((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 130: _t->syncUnlockedDatabase((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 131: { bool _r = _t->syncWithDatabase((*reinterpret_cast< const QSharedPointer<Database>(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 132: _t->uploadAndFinishSync((*reinterpret_cast< const RemoteParams*(*)>(_a[1])),(*reinterpret_cast< RemoteHandler::RemoteResult(*)>(_a[2]))); break;
        case 133: _t->finishSync((*reinterpret_cast< const RemoteParams*(*)>(_a[1])),(*reinterpret_cast< RemoteHandler::RemoteResult(*)>(_a[2]))); break;
        case 134: _t->emitCurrentModeChanged(); break;
        case 135: _t->reloadDatabaseFile((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 136: _t->restoreGroupEntryFocus((*reinterpret_cast< const QUuid(*)>(_a[1])),(*reinterpret_cast< const QUuid(*)>(_a[2]))); break;
        case 137: _t->onConfigChanged((*reinterpret_cast< Config::ConfigKey(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSharedPointer<Database> >(); break;
            }
            break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSharedPointer<Database> >(); break;
            }
            break;
        case 39:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSharedPointer<Database> >(); break;
            }
            break;
        case 45:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<Entry*> >(); break;
            }
            break;
        case 46:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<Entry*> >(); break;
            }
            break;
        case 80:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 81:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 113:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 115:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 116:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 117:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 118:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Group* >(); break;
            }
            break;
        case 121:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Entry* >(); break;
            }
            break;
        case 131:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QSharedPointer<Database> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DatabaseWidget::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseFilePathChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseModified)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseNonDataChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseSaved)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseAboutToUnlock)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseUnlocked)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseLockRequested)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseLocked)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QSharedPointer<Database> & , const QSharedPointer<Database> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseReplaced)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::closeRequest)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(DatabaseWidget::Mode );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::currentModeChanged)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::groupChanged)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::entrySelectionChanged)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QString & , bool , const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::requestOpenDatabase)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(QSharedPointer<Database> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseMerged)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseSyncInProgress)) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseSyncCompleted)) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseSyncFailed)) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const RemoteHandler::RemoteResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseSyncUnlockFailed)) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const RemoteHandler::RemoteResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::databaseSyncUnlocked)) {
                *result = 19;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::unlockDatabaseInDialogForSync)) {
                *result = 20;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(int , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::updateSyncProgress)) {
                *result = 21;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QPoint & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::groupContextMenuRequested)) {
                *result = 22;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QPoint & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::entryContextMenuRequested)) {
                *result = 23;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::listModeAboutToActivate)) {
                *result = 24;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::listModeActivated)) {
                *result = 25;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::searchModeAboutToActivate)) {
                *result = 26;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::searchModeActivated)) {
                *result = 27;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::splitterSizesChanged)) {
                *result = 28;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::entryViewStateChanged)) {
                *result = 29;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::clearSearch)) {
                *result = 30;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::requestGlobalAutoType)) {
                *result = 31;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::requestSearch)) {
                *result = 32;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::reloadBegin)) {
                *result = 33;
                return;
            }
        }
        {
            using _t = void (DatabaseWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseWidget::reloadEnd)) {
                *result = 34;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DatabaseWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QStackedWidget::staticMetaObject>(),
    qt_meta_stringdata_DatabaseWidget.data,
    qt_meta_data_DatabaseWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DatabaseWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DatabaseWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DatabaseWidget.stringdata0))
        return static_cast<void*>(this);
    return QStackedWidget::qt_metacast(_clname);
}

int DatabaseWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QStackedWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 138)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 138;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 138)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 138;
    }
    return _id;
}

// SIGNAL 0
void DatabaseWidget::databaseFilePathChanged(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DatabaseWidget::databaseModified()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void DatabaseWidget::databaseNonDataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void DatabaseWidget::databaseSaved()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void DatabaseWidget::databaseAboutToUnlock()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void DatabaseWidget::databaseUnlocked()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void DatabaseWidget::databaseLockRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void DatabaseWidget::databaseLocked()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void DatabaseWidget::databaseReplaced(const QSharedPointer<Database> & _t1, const QSharedPointer<Database> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void DatabaseWidget::closeRequest()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void DatabaseWidget::currentModeChanged(DatabaseWidget::Mode _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void DatabaseWidget::groupChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void DatabaseWidget::entrySelectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void DatabaseWidget::requestOpenDatabase(const QString & _t1, bool _t2, const QString & _t3, const QString & _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void DatabaseWidget::databaseMerged(QSharedPointer<Database> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void DatabaseWidget::databaseSyncInProgress()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void DatabaseWidget::databaseSyncCompleted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void DatabaseWidget::databaseSyncFailed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void DatabaseWidget::databaseSyncUnlockFailed(const RemoteHandler::RemoteResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void DatabaseWidget::databaseSyncUnlocked(const RemoteHandler::RemoteResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void DatabaseWidget::unlockDatabaseInDialogForSync(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}

// SIGNAL 21
void DatabaseWidget::updateSyncProgress(int _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 21, _a);
}

// SIGNAL 22
void DatabaseWidget::groupContextMenuRequested(const QPoint & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 22, _a);
}

// SIGNAL 23
void DatabaseWidget::entryContextMenuRequested(const QPoint & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 23, _a);
}

// SIGNAL 24
void DatabaseWidget::listModeAboutToActivate()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void DatabaseWidget::listModeActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 25, nullptr);
}

// SIGNAL 26
void DatabaseWidget::searchModeAboutToActivate()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void DatabaseWidget::searchModeActivated()
{
    QMetaObject::activate(this, &staticMetaObject, 27, nullptr);
}

// SIGNAL 28
void DatabaseWidget::splitterSizesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void DatabaseWidget::entryViewStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 29, nullptr);
}

// SIGNAL 30
void DatabaseWidget::clearSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 30, nullptr);
}

// SIGNAL 31
void DatabaseWidget::requestGlobalAutoType(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 31, _a);
}

// SIGNAL 32
void DatabaseWidget::requestSearch(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 32, _a);
}

// SIGNAL 33
void DatabaseWidget::reloadBegin()
{
    QMetaObject::activate(this, &staticMetaObject, 33, nullptr);
}

// SIGNAL 34
void DatabaseWidget::reloadEnd()
{
    QMetaObject::activate(this, &staticMetaObject, 34, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
