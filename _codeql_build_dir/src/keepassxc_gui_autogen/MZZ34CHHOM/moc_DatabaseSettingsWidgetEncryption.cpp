/****************************************************************************
** Meta object code from reading C++ file 'DatabaseSettingsWidgetEncryption.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/dbsettings/DatabaseSettingsWidgetEncryption.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatabaseSettingsWidgetEncryption.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DatabaseSettingsWidgetEncryption_t {
    QByteArrayData data[15];
    char stringdata0[228];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DatabaseSettingsWidgetEncryption_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DatabaseSettingsWidgetEncryption_t qt_meta_stringdata_DatabaseSettingsWidgetEncryption = {
    {
QT_MOC_LITERAL(0, 0, 32), // "DatabaseSettingsWidgetEncryption"
QT_MOC_LITERAL(1, 33, 10), // "initialize"
QT_MOC_LITERAL(2, 44, 0), // ""
QT_MOC_LITERAL(3, 45, 12), // "uninitialize"
QT_MOC_LITERAL(4, 58, 12), // "saveSettings"
QT_MOC_LITERAL(5, 71, 24), // "benchmarkTransformRounds"
QT_MOC_LITERAL(6, 96, 9), // "millisecs"
QT_MOC_LITERAL(7, 106, 13), // "memoryChanged"
QT_MOC_LITERAL(8, 120, 5), // "value"
QT_MOC_LITERAL(9, 126, 18), // "parallelismChanged"
QT_MOC_LITERAL(10, 145, 20), // "updateDecryptionTime"
QT_MOC_LITERAL(11, 166, 17), // "loadKdfAlgorithms"
QT_MOC_LITERAL(12, 184, 17), // "loadKdfParameters"
QT_MOC_LITERAL(13, 202, 15), // "updateKdfFields"
QT_MOC_LITERAL(14, 218, 9) // "markDirty"

    },
    "DatabaseSettingsWidgetEncryption\0"
    "initialize\0\0uninitialize\0saveSettings\0"
    "benchmarkTransformRounds\0millisecs\0"
    "memoryChanged\0value\0parallelismChanged\0"
    "updateDecryptionTime\0loadKdfAlgorithms\0"
    "loadKdfParameters\0updateKdfFields\0"
    "markDirty"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DatabaseSettingsWidgetEncryption[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x0a /* Public */,
       3,    0,   75,    2, 0x0a /* Public */,
       4,    0,   76,    2, 0x0a /* Public */,
       5,    1,   77,    2, 0x08 /* Private */,
       5,    0,   80,    2, 0x28 /* Private | MethodCloned */,
       7,    1,   81,    2, 0x08 /* Private */,
       9,    1,   84,    2, 0x08 /* Private */,
      10,    1,   87,    2, 0x08 /* Private */,
      11,    0,   90,    2, 0x08 /* Private */,
      12,    0,   91,    2, 0x08 /* Private */,
      13,    0,   92,    2, 0x08 /* Private */,
      14,    0,   93,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DatabaseSettingsWidgetEncryption::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DatabaseSettingsWidgetEncryption *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->initialize(); break;
        case 1: _t->uninitialize(); break;
        case 2: { bool _r = _t->saveSettings();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->benchmarkTransformRounds((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->benchmarkTransformRounds(); break;
        case 5: _t->memoryChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->parallelismChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->updateDecryptionTime((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->loadKdfAlgorithms(); break;
        case 9: _t->loadKdfParameters(); break;
        case 10: _t->updateKdfFields(); break;
        case 11: _t->markDirty(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DatabaseSettingsWidgetEncryption::staticMetaObject = { {
    QMetaObject::SuperData::link<DatabaseSettingsWidget::staticMetaObject>(),
    qt_meta_stringdata_DatabaseSettingsWidgetEncryption.data,
    qt_meta_data_DatabaseSettingsWidgetEncryption,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DatabaseSettingsWidgetEncryption::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DatabaseSettingsWidgetEncryption::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DatabaseSettingsWidgetEncryption.stringdata0))
        return static_cast<void*>(this);
    return DatabaseSettingsWidget::qt_metacast(_clname);
}

int DatabaseSettingsWidgetEncryption::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DatabaseSettingsWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
