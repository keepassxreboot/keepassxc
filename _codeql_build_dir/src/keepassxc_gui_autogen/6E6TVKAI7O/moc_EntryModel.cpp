/****************************************************************************
** Meta object code from reading C++ file 'EntryModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/EntryModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryModel_t {
    QByteArrayData data[17];
    char stringdata0[211];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryModel_t qt_meta_stringdata_EntryModel = {
    {
QT_MOC_LITERAL(0, 0, 10), // "EntryModel"
QT_MOC_LITERAL(1, 11, 15), // "entryAboutToAdd"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 6), // "Entry*"
QT_MOC_LITERAL(4, 35, 5), // "entry"
QT_MOC_LITERAL(5, 41, 10), // "entryAdded"
QT_MOC_LITERAL(6, 52, 18), // "entryAboutToRemove"
QT_MOC_LITERAL(7, 71, 12), // "entryRemoved"
QT_MOC_LITERAL(8, 84, 18), // "entryAboutToMoveUp"
QT_MOC_LITERAL(9, 103, 3), // "row"
QT_MOC_LITERAL(10, 107, 12), // "entryMovedUp"
QT_MOC_LITERAL(11, 120, 20), // "entryAboutToMoveDown"
QT_MOC_LITERAL(12, 141, 14), // "entryMovedDown"
QT_MOC_LITERAL(13, 156, 16), // "entryDataChanged"
QT_MOC_LITERAL(14, 173, 15), // "onConfigChanged"
QT_MOC_LITERAL(15, 189, 17), // "Config::ConfigKey"
QT_MOC_LITERAL(16, 207, 3) // "key"

    },
    "EntryModel\0entryAboutToAdd\0\0Entry*\0"
    "entry\0entryAdded\0entryAboutToRemove\0"
    "entryRemoved\0entryAboutToMoveUp\0row\0"
    "entryMovedUp\0entryAboutToMoveDown\0"
    "entryMovedDown\0entryDataChanged\0"
    "onConfigChanged\0Config::ConfigKey\0key"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x08 /* Private */,
       5,    1,   67,    2, 0x08 /* Private */,
       6,    1,   70,    2, 0x08 /* Private */,
       7,    0,   73,    2, 0x08 /* Private */,
       8,    1,   74,    2, 0x08 /* Private */,
      10,    0,   77,    2, 0x08 /* Private */,
      11,    1,   78,    2, 0x08 /* Private */,
      12,    0,   81,    2, 0x08 /* Private */,
      13,    1,   82,    2, 0x08 /* Private */,
      14,    1,   85,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 15,   16,

       0        // eod
};

void EntryModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->entryAboutToAdd((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 1: _t->entryAdded((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 2: _t->entryAboutToRemove((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 3: _t->entryRemoved(); break;
        case 4: _t->entryAboutToMoveUp((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->entryMovedUp(); break;
        case 6: _t->entryAboutToMoveDown((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->entryMovedDown(); break;
        case 8: _t->entryDataChanged((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 9: _t->onConfigChanged((*reinterpret_cast< Config::ConfigKey(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EntryModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractTableModel::staticMetaObject>(),
    qt_meta_stringdata_EntryModel.data,
    qt_meta_data_EntryModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractTableModel::qt_metacast(_clname);
}

int EntryModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractTableModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
