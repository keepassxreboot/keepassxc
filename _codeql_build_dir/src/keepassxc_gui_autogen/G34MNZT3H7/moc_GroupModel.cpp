/****************************************************************************
** Meta object code from reading C++ file 'GroupModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/group/GroupModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GroupModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_GroupModel_t {
    QByteArrayData data[14];
    char stringdata0[147];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GroupModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GroupModel_t qt_meta_stringdata_GroupModel = {
    {
QT_MOC_LITERAL(0, 0, 10), // "GroupModel"
QT_MOC_LITERAL(1, 11, 16), // "groupDataChanged"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 6), // "Group*"
QT_MOC_LITERAL(4, 36, 5), // "group"
QT_MOC_LITERAL(5, 42, 18), // "groupAboutToRemove"
QT_MOC_LITERAL(6, 61, 12), // "groupRemoved"
QT_MOC_LITERAL(7, 74, 15), // "groupAboutToAdd"
QT_MOC_LITERAL(8, 90, 5), // "index"
QT_MOC_LITERAL(9, 96, 10), // "groupAdded"
QT_MOC_LITERAL(10, 107, 16), // "groupAboutToMove"
QT_MOC_LITERAL(11, 124, 7), // "toGroup"
QT_MOC_LITERAL(12, 132, 3), // "pos"
QT_MOC_LITERAL(13, 136, 10) // "groupMoved"

    },
    "GroupModel\0groupDataChanged\0\0Group*\0"
    "group\0groupAboutToRemove\0groupRemoved\0"
    "groupAboutToAdd\0index\0groupAdded\0"
    "groupAboutToMove\0toGroup\0pos\0groupMoved"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GroupModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x08 /* Private */,
       5,    1,   52,    2, 0x08 /* Private */,
       6,    0,   55,    2, 0x08 /* Private */,
       7,    2,   56,    2, 0x08 /* Private */,
       9,    0,   61,    2, 0x08 /* Private */,
      10,    3,   62,    2, 0x08 /* Private */,
      13,    0,   69,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    8,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3, QMetaType::Int,    4,   11,   12,
    QMetaType::Void,

       0        // eod
};

void GroupModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GroupModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->groupDataChanged((*reinterpret_cast< Group*(*)>(_a[1]))); break;
        case 1: _t->groupAboutToRemove((*reinterpret_cast< Group*(*)>(_a[1]))); break;
        case 2: _t->groupRemoved(); break;
        case 3: _t->groupAboutToAdd((*reinterpret_cast< Group*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->groupAdded(); break;
        case 5: _t->groupAboutToMove((*reinterpret_cast< Group*(*)>(_a[1])),(*reinterpret_cast< Group*(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 6: _t->groupMoved(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject GroupModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractItemModel::staticMetaObject>(),
    qt_meta_stringdata_GroupModel.data,
    qt_meta_data_GroupModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *GroupModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GroupModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GroupModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractItemModel::qt_metacast(_clname);
}

int GroupModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractItemModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
