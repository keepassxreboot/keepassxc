/****************************************************************************
** Meta object code from reading C++ file 'AutoType.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/autotype/AutoType.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AutoType.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AutoType_t {
    QByteArrayData data[13];
    char stringdata0[206];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AutoType_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AutoType_t qt_meta_stringdata_AutoType = {
    {
QT_MOC_LITERAL(0, 0, 8), // "AutoType"
QT_MOC_LITERAL(1, 9, 23), // "globalAutoTypeTriggered"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 6), // "search"
QT_MOC_LITERAL(4, 41, 16), // "autotypeFinished"
QT_MOC_LITERAL(5, 58, 21), // "autotypeRetypeTimeout"
QT_MOC_LITERAL(6, 80, 21), // "performGlobalAutoType"
QT_MOC_LITERAL(7, 102, 32), // "QList<QSharedPointer<Database> >"
QT_MOC_LITERAL(8, 135, 6), // "dbList"
QT_MOC_LITERAL(9, 142, 11), // "raiseWindow"
QT_MOC_LITERAL(10, 154, 19), // "startGlobalAutoType"
QT_MOC_LITERAL(11, 174, 12), // "unloadPlugin"
QT_MOC_LITERAL(12, 187, 18) // "resetAutoTypeState"

    },
    "AutoType\0globalAutoTypeTriggered\0\0"
    "search\0autotypeFinished\0autotypeRetypeTimeout\0"
    "performGlobalAutoType\0"
    "QList<QSharedPointer<Database> >\0"
    "dbList\0raiseWindow\0startGlobalAutoType\0"
    "unloadPlugin\0resetAutoTypeState"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AutoType[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    0,   62,    2, 0x06 /* Public */,
       5,    0,   63,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    2,   64,    2, 0x0a /* Public */,
       6,    1,   69,    2, 0x2a /* Public | MethodCloned */,
       9,    0,   72,    2, 0x0a /* Public */,
      10,    1,   73,    2, 0x08 /* Private */,
      11,    0,   76,    2, 0x08 /* Private */,
      12,    0,   77,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 7, QMetaType::QString,    8,    3,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AutoType::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AutoType *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->globalAutoTypeTriggered((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->autotypeFinished(); break;
        case 2: _t->autotypeRetypeTimeout(); break;
        case 3: _t->performGlobalAutoType((*reinterpret_cast< const QList<QSharedPointer<Database> >(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->performGlobalAutoType((*reinterpret_cast< const QList<QSharedPointer<Database> >(*)>(_a[1]))); break;
        case 5: _t->raiseWindow(); break;
        case 6: _t->startGlobalAutoType((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->unloadPlugin(); break;
        case 8: _t->resetAutoTypeState(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AutoType::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoType::globalAutoTypeTriggered)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AutoType::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoType::autotypeFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AutoType::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoType::autotypeRetypeTimeout)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AutoType::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_AutoType.data,
    qt_meta_data_AutoType,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AutoType::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AutoType::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AutoType.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AutoType::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void AutoType::globalAutoTypeTriggered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AutoType::autotypeFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AutoType::autotypeRetypeTimeout()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
