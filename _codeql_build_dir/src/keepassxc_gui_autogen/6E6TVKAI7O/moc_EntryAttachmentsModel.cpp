/****************************************************************************
** Meta object code from reading C++ file 'EntryAttachmentsModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/EntryAttachmentsModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryAttachmentsModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryAttachmentsModel_t {
    QByteArrayData data[12];
    char stringdata0[160];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryAttachmentsModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryAttachmentsModel_t qt_meta_stringdata_EntryAttachmentsModel = {
    {
QT_MOC_LITERAL(0, 0, 21), // "EntryAttachmentsModel"
QT_MOC_LITERAL(1, 22, 16), // "attachmentChange"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 3), // "key"
QT_MOC_LITERAL(4, 44, 20), // "attachmentAboutToAdd"
QT_MOC_LITERAL(5, 65, 13), // "attachmentAdd"
QT_MOC_LITERAL(6, 79, 23), // "attachmentAboutToRemove"
QT_MOC_LITERAL(7, 103, 16), // "attachmentRemove"
QT_MOC_LITERAL(8, 120, 12), // "aboutToReset"
QT_MOC_LITERAL(9, 133, 5), // "reset"
QT_MOC_LITERAL(10, 139, 11), // "setReadOnly"
QT_MOC_LITERAL(11, 151, 8) // "readOnly"

    },
    "EntryAttachmentsModel\0attachmentChange\0"
    "\0key\0attachmentAboutToAdd\0attachmentAdd\0"
    "attachmentAboutToRemove\0attachmentRemove\0"
    "aboutToReset\0reset\0setReadOnly\0readOnly"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryAttachmentsModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x08 /* Private */,
       4,    1,   57,    2, 0x08 /* Private */,
       5,    0,   60,    2, 0x08 /* Private */,
       6,    1,   61,    2, 0x08 /* Private */,
       7,    0,   64,    2, 0x08 /* Private */,
       8,    0,   65,    2, 0x08 /* Private */,
       9,    0,   66,    2, 0x08 /* Private */,
      10,    1,   67,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   11,

       0        // eod
};

void EntryAttachmentsModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryAttachmentsModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->attachmentChange((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->attachmentAboutToAdd((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->attachmentAdd(); break;
        case 3: _t->attachmentAboutToRemove((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->attachmentRemove(); break;
        case 5: _t->aboutToReset(); break;
        case 6: _t->reset(); break;
        case 7: _t->setReadOnly((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EntryAttachmentsModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_meta_stringdata_EntryAttachmentsModel.data,
    qt_meta_data_EntryAttachmentsModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryAttachmentsModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryAttachmentsModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryAttachmentsModel.stringdata0))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int EntryAttachmentsModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
