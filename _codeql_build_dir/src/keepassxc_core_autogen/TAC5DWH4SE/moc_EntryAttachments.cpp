/****************************************************************************
** Meta object code from reading C++ file 'EntryAttachments.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/core/EntryAttachments.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryAttachments.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryAttachments_t {
    QByteArrayData data[13];
    char stringdata0[153];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryAttachments_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryAttachments_t qt_meta_stringdata_EntryAttachments = {
    {
QT_MOC_LITERAL(0, 0, 16), // "EntryAttachments"
QT_MOC_LITERAL(1, 17, 11), // "keyModified"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 3), // "key"
QT_MOC_LITERAL(4, 34, 23), // "valueModifiedExternally"
QT_MOC_LITERAL(5, 58, 4), // "path"
QT_MOC_LITERAL(6, 63, 14), // "aboutToBeAdded"
QT_MOC_LITERAL(7, 78, 5), // "added"
QT_MOC_LITERAL(8, 84, 16), // "aboutToBeRemoved"
QT_MOC_LITERAL(9, 101, 7), // "removed"
QT_MOC_LITERAL(10, 109, 14), // "aboutToBeReset"
QT_MOC_LITERAL(11, 124, 5), // "reset"
QT_MOC_LITERAL(12, 130, 22) // "attachmentFileModified"

    },
    "EntryAttachments\0keyModified\0\0key\0"
    "valueModifiedExternally\0path\0"
    "aboutToBeAdded\0added\0aboutToBeRemoved\0"
    "removed\0aboutToBeReset\0reset\0"
    "attachmentFileModified"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryAttachments[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    2,   62,    2, 0x06 /* Public */,
       6,    1,   67,    2, 0x06 /* Public */,
       7,    1,   70,    2, 0x06 /* Public */,
       8,    1,   73,    2, 0x06 /* Public */,
       9,    1,   76,    2, 0x06 /* Public */,
      10,    0,   79,    2, 0x06 /* Public */,
      11,    0,   80,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,   81,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    5,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    5,

       0        // eod
};

void EntryAttachments::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryAttachments *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->keyModified((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->valueModifiedExternally((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->aboutToBeAdded((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->added((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->aboutToBeRemoved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->removed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->aboutToBeReset(); break;
        case 7: _t->reset(); break;
        case 8: _t->attachmentFileModified((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EntryAttachments::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::keyModified)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::valueModifiedExternally)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::aboutToBeAdded)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::added)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::aboutToBeRemoved)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::removed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::aboutToBeReset)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (EntryAttachments::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachments::reset)) {
                *result = 7;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EntryAttachments::staticMetaObject = { {
    QMetaObject::SuperData::link<ModifiableObject::staticMetaObject>(),
    qt_meta_stringdata_EntryAttachments.data,
    qt_meta_data_EntryAttachments,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryAttachments::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryAttachments::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryAttachments.stringdata0))
        return static_cast<void*>(this);
    return ModifiableObject::qt_metacast(_clname);
}

int EntryAttachments::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ModifiableObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void EntryAttachments::keyModified(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EntryAttachments::valueModifiedExternally(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EntryAttachments::aboutToBeAdded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void EntryAttachments::added(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void EntryAttachments::aboutToBeRemoved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void EntryAttachments::removed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void EntryAttachments::aboutToBeReset()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void EntryAttachments::reset()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
