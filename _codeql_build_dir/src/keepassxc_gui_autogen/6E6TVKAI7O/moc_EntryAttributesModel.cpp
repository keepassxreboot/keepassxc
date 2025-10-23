/****************************************************************************
** Meta object code from reading C++ file 'EntryAttributesModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/EntryAttributesModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryAttributesModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryAttributesModel_t {
    QByteArrayData data[14];
    char stringdata0[186];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryAttributesModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryAttributesModel_t qt_meta_stringdata_EntryAttributesModel = {
    {
QT_MOC_LITERAL(0, 0, 20), // "EntryAttributesModel"
QT_MOC_LITERAL(1, 21, 15), // "attributeChange"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 3), // "key"
QT_MOC_LITERAL(4, 42, 19), // "attributeAboutToAdd"
QT_MOC_LITERAL(5, 62, 12), // "attributeAdd"
QT_MOC_LITERAL(6, 75, 22), // "attributeAboutToRemove"
QT_MOC_LITERAL(7, 98, 15), // "attributeRemove"
QT_MOC_LITERAL(8, 114, 22), // "attributeAboutToRename"
QT_MOC_LITERAL(9, 137, 6), // "oldKey"
QT_MOC_LITERAL(10, 144, 6), // "newKey"
QT_MOC_LITERAL(11, 151, 15), // "attributeRename"
QT_MOC_LITERAL(12, 167, 12), // "aboutToReset"
QT_MOC_LITERAL(13, 180, 5) // "reset"

    },
    "EntryAttributesModel\0attributeChange\0"
    "\0key\0attributeAboutToAdd\0attributeAdd\0"
    "attributeAboutToRemove\0attributeRemove\0"
    "attributeAboutToRename\0oldKey\0newKey\0"
    "attributeRename\0aboutToReset\0reset"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryAttributesModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x08 /* Private */,
       4,    1,   62,    2, 0x08 /* Private */,
       5,    0,   65,    2, 0x08 /* Private */,
       6,    1,   66,    2, 0x08 /* Private */,
       7,    0,   69,    2, 0x08 /* Private */,
       8,    2,   70,    2, 0x08 /* Private */,
      11,    2,   75,    2, 0x08 /* Private */,
      12,    0,   80,    2, 0x08 /* Private */,
      13,    0,   81,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   10,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EntryAttributesModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryAttributesModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->attributeChange((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->attributeAboutToAdd((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->attributeAdd(); break;
        case 3: _t->attributeAboutToRemove((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->attributeRemove(); break;
        case 5: _t->attributeAboutToRename((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 6: _t->attributeRename((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 7: _t->aboutToReset(); break;
        case 8: _t->reset(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EntryAttributesModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_meta_stringdata_EntryAttributesModel.data,
    qt_meta_data_EntryAttributesModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryAttributesModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryAttributesModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryAttributesModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int EntryAttributesModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
