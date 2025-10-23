/****************************************************************************
** Meta object code from reading C++ file 'NixUtils.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/osutils/nixutils/NixUtils.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NixUtils.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NixUtils_t {
    QByteArrayData data[11];
    char stringdata0[125];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NixUtils_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NixUtils_t qt_meta_stringdata_NixUtils = {
    {
QT_MOC_LITERAL(0, 0, 8), // "NixUtils"
QT_MOC_LITERAL(1, 9, 21), // "handleColorSchemeRead"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 12), // "QDBusVariant"
QT_MOC_LITERAL(4, 45, 5), // "value"
QT_MOC_LITERAL(5, 51, 24), // "handleColorSchemeChanged"
QT_MOC_LITERAL(6, 76, 2), // "ns"
QT_MOC_LITERAL(7, 79, 3), // "key"
QT_MOC_LITERAL(8, 83, 24), // "launchAtStartupRequested"
QT_MOC_LITERAL(9, 108, 8), // "response"
QT_MOC_LITERAL(10, 117, 7) // "results"

    },
    "NixUtils\0handleColorSchemeRead\0\0"
    "QDBusVariant\0value\0handleColorSchemeChanged\0"
    "ns\0key\0launchAtStartupRequested\0"
    "response\0results"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NixUtils[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x08 /* Private */,
       5,    3,   32,    2, 0x08 /* Private */,
       8,    2,   39,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, 0x80000000 | 3,    6,    7,    4,
    QMetaType::Void, QMetaType::UInt, QMetaType::QVariantMap,    9,   10,

       0        // eod
};

void NixUtils::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NixUtils *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->handleColorSchemeRead((*reinterpret_cast< QDBusVariant(*)>(_a[1]))); break;
        case 1: _t->handleColorSchemeChanged((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QDBusVariant(*)>(_a[3]))); break;
        case 2: _t->launchAtStartupRequested((*reinterpret_cast< uint(*)>(_a[1])),(*reinterpret_cast< const QVariantMap(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QDBusVariant >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 2:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QDBusVariant >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NixUtils::staticMetaObject = { {
    QMetaObject::SuperData::link<OSUtilsBase::staticMetaObject>(),
    qt_meta_stringdata_NixUtils.data,
    qt_meta_data_NixUtils,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NixUtils::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NixUtils::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NixUtils.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QAbstractNativeEventFilter"))
        return static_cast< QAbstractNativeEventFilter*>(this);
    return OSUtilsBase::qt_metacast(_clname);
}

int NixUtils::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = OSUtilsBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
