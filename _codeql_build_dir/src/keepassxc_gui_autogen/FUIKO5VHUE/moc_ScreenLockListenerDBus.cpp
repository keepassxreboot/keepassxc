/****************************************************************************
** Meta object code from reading C++ file 'ScreenLockListenerDBus.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/osutils/nixutils/ScreenLockListenerDBus.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ScreenLockListenerDBus.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ScreenLockListenerDBus_t {
    QByteArrayData data[10];
    char stringdata0[167];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScreenLockListenerDBus_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScreenLockListenerDBus_t qt_meta_stringdata_ScreenLockListenerDBus = {
    {
QT_MOC_LITERAL(0, 0, 22), // "ScreenLockListenerDBus"
QT_MOC_LITERAL(1, 23, 25), // "gnomeSessionStatusChanged"
QT_MOC_LITERAL(2, 49, 0), // ""
QT_MOC_LITERAL(3, 50, 6), // "status"
QT_MOC_LITERAL(4, 57, 21), // "logindPrepareForSleep"
QT_MOC_LITERAL(5, 79, 11), // "beforeSleep"
QT_MOC_LITERAL(6, 91, 11), // "unityLocked"
QT_MOC_LITERAL(7, 103, 22), // "freedesktopScreenSaver"
QT_MOC_LITERAL(8, 126, 27), // "login1SessionObjectReceived"
QT_MOC_LITERAL(9, 154, 12) // "QDBusMessage"

    },
    "ScreenLockListenerDBus\0gnomeSessionStatusChanged\0"
    "\0status\0logindPrepareForSleep\0beforeSleep\0"
    "unityLocked\0freedesktopScreenSaver\0"
    "login1SessionObjectReceived\0QDBusMessage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScreenLockListenerDBus[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x08 /* Private */,
       4,    1,   42,    2, 0x08 /* Private */,
       6,    0,   45,    2, 0x08 /* Private */,
       7,    1,   46,    2, 0x08 /* Private */,
       8,    1,   49,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::UInt,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, 0x80000000 | 9,    2,

       0        // eod
};

void ScreenLockListenerDBus::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScreenLockListenerDBus *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->gnomeSessionStatusChanged((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 1: _t->logindPrepareForSleep((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->unityLocked(); break;
        case 3: _t->freedesktopScreenSaver((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->login1SessionObjectReceived((*reinterpret_cast< QDBusMessage(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QDBusMessage >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ScreenLockListenerDBus::staticMetaObject = { {
    QMetaObject::SuperData::link<ScreenLockListenerPrivate::staticMetaObject>(),
    qt_meta_stringdata_ScreenLockListenerDBus.data,
    qt_meta_data_ScreenLockListenerDBus,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ScreenLockListenerDBus::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScreenLockListenerDBus::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScreenLockListenerDBus.stringdata0))
        return static_cast<void*>(this);
    return ScreenLockListenerPrivate::qt_metacast(_clname);
}

int ScreenLockListenerDBus::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ScreenLockListenerPrivate::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
