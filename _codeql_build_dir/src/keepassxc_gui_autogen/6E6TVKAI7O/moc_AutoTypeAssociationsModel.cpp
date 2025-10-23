/****************************************************************************
** Meta object code from reading C++ file 'AutoTypeAssociationsModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/AutoTypeAssociationsModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AutoTypeAssociationsModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AutoTypeAssociationsModel_t {
    QByteArrayData data[10];
    char stringdata0[146];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AutoTypeAssociationsModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AutoTypeAssociationsModel_t qt_meta_stringdata_AutoTypeAssociationsModel = {
    {
QT_MOC_LITERAL(0, 0, 25), // "AutoTypeAssociationsModel"
QT_MOC_LITERAL(1, 26, 17), // "associationChange"
QT_MOC_LITERAL(2, 44, 0), // ""
QT_MOC_LITERAL(3, 45, 1), // "i"
QT_MOC_LITERAL(4, 47, 21), // "associationAboutToAdd"
QT_MOC_LITERAL(5, 69, 14), // "associationAdd"
QT_MOC_LITERAL(6, 84, 24), // "associationAboutToRemove"
QT_MOC_LITERAL(7, 109, 17), // "associationRemove"
QT_MOC_LITERAL(8, 127, 12), // "aboutToReset"
QT_MOC_LITERAL(9, 140, 5) // "reset"

    },
    "AutoTypeAssociationsModel\0associationChange\0"
    "\0i\0associationAboutToAdd\0associationAdd\0"
    "associationAboutToRemove\0associationRemove\0"
    "aboutToReset\0reset"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AutoTypeAssociationsModel[] = {

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
       1,    1,   49,    2, 0x0a /* Public */,
       4,    1,   52,    2, 0x0a /* Public */,
       5,    0,   55,    2, 0x0a /* Public */,
       6,    1,   56,    2, 0x0a /* Public */,
       7,    0,   59,    2, 0x0a /* Public */,
       8,    0,   60,    2, 0x0a /* Public */,
       9,    0,   61,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AutoTypeAssociationsModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AutoTypeAssociationsModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->associationChange((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->associationAboutToAdd((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->associationAdd(); break;
        case 3: _t->associationAboutToRemove((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->associationRemove(); break;
        case 5: _t->aboutToReset(); break;
        case 6: _t->reset(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AutoTypeAssociationsModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_meta_stringdata_AutoTypeAssociationsModel.data,
    qt_meta_data_AutoTypeAssociationsModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AutoTypeAssociationsModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AutoTypeAssociationsModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AutoTypeAssociationsModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int AutoTypeAssociationsModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
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
