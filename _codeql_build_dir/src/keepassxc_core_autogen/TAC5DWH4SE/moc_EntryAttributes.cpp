/****************************************************************************
** Meta object code from reading C++ file 'EntryAttributes.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/core/EntryAttributes.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryAttributes.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryAttributes_t {
    QByteArrayData data[15];
    char stringdata0[161];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryAttributes_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryAttributes_t qt_meta_stringdata_EntryAttributes = {
    {
QT_MOC_LITERAL(0, 0, 15), // "EntryAttributes"
QT_MOC_LITERAL(1, 16, 18), // "defaultKeyModified"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 17), // "customKeyModified"
QT_MOC_LITERAL(4, 54, 3), // "key"
QT_MOC_LITERAL(5, 58, 14), // "aboutToBeAdded"
QT_MOC_LITERAL(6, 73, 5), // "added"
QT_MOC_LITERAL(7, 79, 16), // "aboutToBeRemoved"
QT_MOC_LITERAL(8, 96, 7), // "removed"
QT_MOC_LITERAL(9, 104, 13), // "aboutToRename"
QT_MOC_LITERAL(10, 118, 6), // "oldKey"
QT_MOC_LITERAL(11, 125, 6), // "newKey"
QT_MOC_LITERAL(12, 132, 7), // "renamed"
QT_MOC_LITERAL(13, 140, 14), // "aboutToBeReset"
QT_MOC_LITERAL(14, 155, 5) // "reset"

    },
    "EntryAttributes\0defaultKeyModified\0\0"
    "customKeyModified\0key\0aboutToBeAdded\0"
    "added\0aboutToBeRemoved\0removed\0"
    "aboutToRename\0oldKey\0newKey\0renamed\0"
    "aboutToBeReset\0reset"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryAttributes[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   64,    2, 0x06 /* Public */,
       3,    1,   65,    2, 0x06 /* Public */,
       5,    1,   68,    2, 0x06 /* Public */,
       6,    1,   71,    2, 0x06 /* Public */,
       7,    1,   74,    2, 0x06 /* Public */,
       8,    1,   77,    2, 0x06 /* Public */,
       9,    2,   80,    2, 0x06 /* Public */,
      12,    2,   85,    2, 0x06 /* Public */,
      13,    0,   90,    2, 0x06 /* Public */,
      14,    0,   91,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   11,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   10,   11,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EntryAttributes::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryAttributes *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->defaultKeyModified(); break;
        case 1: _t->customKeyModified((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->aboutToBeAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->added((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->aboutToBeRemoved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->removed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->aboutToRename((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 7: _t->renamed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 8: _t->aboutToBeReset(); break;
        case 9: _t->reset(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EntryAttributes::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::defaultKeyModified)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::customKeyModified)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::aboutToBeAdded)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::added)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::aboutToBeRemoved)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::removed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::aboutToRename)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::renamed)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::aboutToBeReset)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (EntryAttributes::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttributes::reset)) {
                *result = 9;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EntryAttributes::staticMetaObject = { {
    QMetaObject::SuperData::link<ModifiableObject::staticMetaObject>(),
    qt_meta_stringdata_EntryAttributes.data,
    qt_meta_data_EntryAttributes,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryAttributes::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryAttributes::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryAttributes.stringdata0))
        return static_cast<void*>(this);
    return ModifiableObject::qt_metacast(_clname);
}

int EntryAttributes::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void EntryAttributes::defaultKeyModified()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void EntryAttributes::customKeyModified(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EntryAttributes::aboutToBeAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void EntryAttributes::added(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void EntryAttributes::aboutToBeRemoved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void EntryAttributes::removed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void EntryAttributes::aboutToRename(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void EntryAttributes::renamed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void EntryAttributes::aboutToBeReset()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void EntryAttributes::reset()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
