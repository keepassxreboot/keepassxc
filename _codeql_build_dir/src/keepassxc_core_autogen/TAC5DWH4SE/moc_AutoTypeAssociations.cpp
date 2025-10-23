/****************************************************************************
** Meta object code from reading C++ file 'AutoTypeAssociations.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/core/AutoTypeAssociations.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AutoTypeAssociations.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AutoTypeAssociations_t {
    QByteArrayData data[10];
    char stringdata0[98];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AutoTypeAssociations_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AutoTypeAssociations_t qt_meta_stringdata_AutoTypeAssociations = {
    {
QT_MOC_LITERAL(0, 0, 20), // "AutoTypeAssociations"
QT_MOC_LITERAL(1, 21, 11), // "dataChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 5), // "index"
QT_MOC_LITERAL(4, 40, 10), // "aboutToAdd"
QT_MOC_LITERAL(5, 51, 5), // "added"
QT_MOC_LITERAL(6, 57, 13), // "aboutToRemove"
QT_MOC_LITERAL(7, 71, 7), // "removed"
QT_MOC_LITERAL(8, 79, 12), // "aboutToReset"
QT_MOC_LITERAL(9, 92, 5) // "reset"

    },
    "AutoTypeAssociations\0dataChanged\0\0"
    "index\0aboutToAdd\0added\0aboutToRemove\0"
    "removed\0aboutToReset\0reset"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AutoTypeAssociations[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       4,    1,   52,    2, 0x06 /* Public */,
       5,    1,   55,    2, 0x06 /* Public */,
       6,    1,   58,    2, 0x06 /* Public */,
       7,    1,   61,    2, 0x06 /* Public */,
       8,    0,   64,    2, 0x06 /* Public */,
       9,    0,   65,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AutoTypeAssociations::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AutoTypeAssociations *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->dataChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->aboutToAdd((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->added((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->aboutToRemove((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->removed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->aboutToReset(); break;
        case 6: _t->reset(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AutoTypeAssociations::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::dataChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AutoTypeAssociations::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::aboutToAdd)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AutoTypeAssociations::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::added)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AutoTypeAssociations::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::aboutToRemove)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AutoTypeAssociations::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::removed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (AutoTypeAssociations::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::aboutToReset)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (AutoTypeAssociations::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeAssociations::reset)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AutoTypeAssociations::staticMetaObject = { {
    QMetaObject::SuperData::link<ModifiableObject::staticMetaObject>(),
    qt_meta_stringdata_AutoTypeAssociations.data,
    qt_meta_data_AutoTypeAssociations,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AutoTypeAssociations::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AutoTypeAssociations::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AutoTypeAssociations.stringdata0))
        return static_cast<void*>(this);
    return ModifiableObject::qt_metacast(_clname);
}

int AutoTypeAssociations::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ModifiableObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void AutoTypeAssociations::dataChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AutoTypeAssociations::aboutToAdd(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void AutoTypeAssociations::added(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AutoTypeAssociations::aboutToRemove(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void AutoTypeAssociations::removed(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void AutoTypeAssociations::aboutToReset()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AutoTypeAssociations::reset()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
