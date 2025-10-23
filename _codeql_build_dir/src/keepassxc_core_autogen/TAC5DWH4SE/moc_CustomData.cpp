/****************************************************************************
** Meta object code from reading C++ file 'CustomData.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/core/CustomData.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CustomData.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CustomData_t {
    QByteArrayData data[15];
    char stringdata0[151];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CustomData_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CustomData_t qt_meta_stringdata_CustomData = {
    {
QT_MOC_LITERAL(0, 0, 10), // "CustomData"
QT_MOC_LITERAL(1, 11, 14), // "aboutToBeAdded"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 3), // "key"
QT_MOC_LITERAL(4, 31, 5), // "added"
QT_MOC_LITERAL(5, 37, 16), // "aboutToBeRemoved"
QT_MOC_LITERAL(6, 54, 7), // "removed"
QT_MOC_LITERAL(7, 62, 13), // "aboutToRename"
QT_MOC_LITERAL(8, 76, 6), // "oldKey"
QT_MOC_LITERAL(9, 83, 6), // "newKey"
QT_MOC_LITERAL(10, 90, 7), // "renamed"
QT_MOC_LITERAL(11, 98, 14), // "aboutToBeReset"
QT_MOC_LITERAL(12, 113, 5), // "reset"
QT_MOC_LITERAL(13, 119, 18), // "updateLastModified"
QT_MOC_LITERAL(14, 138, 12) // "lastModified"

    },
    "CustomData\0aboutToBeAdded\0\0key\0added\0"
    "aboutToBeRemoved\0removed\0aboutToRename\0"
    "oldKey\0newKey\0renamed\0aboutToBeReset\0"
    "reset\0updateLastModified\0lastModified"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CustomData[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x06 /* Public */,
       4,    1,   67,    2, 0x06 /* Public */,
       5,    1,   70,    2, 0x06 /* Public */,
       6,    1,   73,    2, 0x06 /* Public */,
       7,    2,   76,    2, 0x06 /* Public */,
      10,    2,   81,    2, 0x06 /* Public */,
      11,    0,   86,    2, 0x06 /* Public */,
      12,    0,   87,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    1,   88,    2, 0x08 /* Private */,
      13,    0,   91,    2, 0x28 /* Private | MethodCloned */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    8,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    8,    9,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QDateTime,   14,
    QMetaType::Void,

       0        // eod
};

void CustomData::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CustomData *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->aboutToBeAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->added((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->aboutToBeRemoved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->removed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->aboutToRename((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 5: _t->renamed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 6: _t->aboutToBeReset(); break;
        case 7: _t->reset(); break;
        case 8: _t->updateLastModified((*reinterpret_cast< QDateTime(*)>(_a[1]))); break;
        case 9: _t->updateLastModified(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CustomData::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::aboutToBeAdded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CustomData::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::added)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CustomData::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::aboutToBeRemoved)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CustomData::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::removed)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CustomData::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::aboutToRename)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CustomData::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::renamed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (CustomData::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::aboutToBeReset)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (CustomData::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomData::reset)) {
                *result = 7;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CustomData::staticMetaObject = { {
    QMetaObject::SuperData::link<ModifiableObject::staticMetaObject>(),
    qt_meta_stringdata_CustomData.data,
    qt_meta_data_CustomData,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CustomData::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CustomData::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CustomData.stringdata0))
        return static_cast<void*>(this);
    return ModifiableObject::qt_metacast(_clname);
}

int CustomData::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ModifiableObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void CustomData::aboutToBeAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CustomData::added(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CustomData::aboutToBeRemoved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CustomData::removed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CustomData::aboutToRename(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CustomData::renamed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CustomData::aboutToBeReset()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void CustomData::reset()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
