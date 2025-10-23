/****************************************************************************
** Meta object code from reading C++ file 'ApplicationSettingsWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/ApplicationSettingsWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ApplicationSettingsWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ApplicationSettingsWidget_t {
    QByteArrayData data[17];
    char stringdata0[304];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ApplicationSettingsWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ApplicationSettingsWidget_t qt_meta_stringdata_ApplicationSettingsWidget = {
    {
QT_MOC_LITERAL(0, 0, 25), // "ApplicationSettingsWidget"
QT_MOC_LITERAL(1, 26, 13), // "settingsReset"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 12), // "saveSettings"
QT_MOC_LITERAL(4, 54, 13), // "resetSettings"
QT_MOC_LITERAL(5, 68, 14), // "importSettings"
QT_MOC_LITERAL(6, 83, 14), // "exportSettings"
QT_MOC_LITERAL(7, 98, 6), // "reject"
QT_MOC_LITERAL(8, 105, 15), // "autoSaveToggled"
QT_MOC_LITERAL(9, 121, 7), // "checked"
QT_MOC_LITERAL(10, 129, 31), // "hideWindowOnCopyCheckBoxToggled"
QT_MOC_LITERAL(11, 161, 14), // "systrayToggled"
QT_MOC_LITERAL(12, 176, 24), // "rememberDatabasesToggled"
QT_MOC_LITERAL(13, 201, 19), // "checkUpdatesToggled"
QT_MOC_LITERAL(14, 221, 41), // "showExpiredEntriesOnDatabaseU..."
QT_MOC_LITERAL(15, 263, 18), // "autoTypeAskToggled"
QT_MOC_LITERAL(16, 282, 21) // "selectBackupDirectory"

    },
    "ApplicationSettingsWidget\0settingsReset\0"
    "\0saveSettings\0resetSettings\0importSettings\0"
    "exportSettings\0reject\0autoSaveToggled\0"
    "checked\0hideWindowOnCopyCheckBoxToggled\0"
    "systrayToggled\0rememberDatabasesToggled\0"
    "checkUpdatesToggled\0"
    "showExpiredEntriesOnDatabaseUnlockToggled\0"
    "autoTypeAskToggled\0selectBackupDirectory"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ApplicationSettingsWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   84,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   85,    2, 0x08 /* Private */,
       4,    0,   86,    2, 0x08 /* Private */,
       5,    0,   87,    2, 0x08 /* Private */,
       6,    0,   88,    2, 0x08 /* Private */,
       7,    0,   89,    2, 0x08 /* Private */,
       8,    1,   90,    2, 0x08 /* Private */,
      10,    1,   93,    2, 0x08 /* Private */,
      11,    1,   96,    2, 0x08 /* Private */,
      12,    1,   99,    2, 0x08 /* Private */,
      13,    1,  102,    2, 0x08 /* Private */,
      14,    1,  105,    2, 0x08 /* Private */,
      15,    1,  108,    2, 0x08 /* Private */,
      16,    0,  111,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,

       0        // eod
};

void ApplicationSettingsWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ApplicationSettingsWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->settingsReset(); break;
        case 1: _t->saveSettings(); break;
        case 2: _t->resetSettings(); break;
        case 3: _t->importSettings(); break;
        case 4: _t->exportSettings(); break;
        case 5: _t->reject(); break;
        case 6: _t->autoSaveToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->hideWindowOnCopyCheckBoxToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->systrayToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->rememberDatabasesToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->checkUpdatesToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->showExpiredEntriesOnDatabaseUnlockToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->autoTypeAskToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->selectBackupDirectory(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ApplicationSettingsWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ApplicationSettingsWidget::settingsReset)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ApplicationSettingsWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<EditWidget::staticMetaObject>(),
    qt_meta_stringdata_ApplicationSettingsWidget.data,
    qt_meta_data_ApplicationSettingsWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ApplicationSettingsWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ApplicationSettingsWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ApplicationSettingsWidget.stringdata0))
        return static_cast<void*>(this);
    return EditWidget::qt_metacast(_clname);
}

int ApplicationSettingsWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = EditWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void ApplicationSettingsWidget::settingsReset()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
