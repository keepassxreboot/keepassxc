/****************************************************************************
** Meta object code from reading C++ file 'DatabaseOpenWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/DatabaseOpenWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatabaseOpenWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DatabaseOpenWidget_t {
    QByteArrayData data[15];
    char stringdata0[187];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DatabaseOpenWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DatabaseOpenWidget_t qt_meta_stringdata_DatabaseOpenWidget = {
    {
QT_MOC_LITERAL(0, 0, 18), // "DatabaseOpenWidget"
QT_MOC_LITERAL(1, 19, 14), // "dialogFinished"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 8), // "accepted"
QT_MOC_LITERAL(4, 44, 12), // "openDatabase"
QT_MOC_LITERAL(5, 57, 6), // "reject"
QT_MOC_LITERAL(6, 64, 13), // "browseKeyFile"
QT_MOC_LITERAL(7, 78, 26), // "toggleHardwareKeyComponent"
QT_MOC_LITERAL(8, 105, 5), // "state"
QT_MOC_LITERAL(9, 111, 13), // "closeDatabase"
QT_MOC_LITERAL(10, 125, 15), // "pollHardwareKey"
QT_MOC_LITERAL(11, 141, 13), // "manualTrigger"
QT_MOC_LITERAL(12, 155, 5), // "delay"
QT_MOC_LITERAL(13, 161, 19), // "hardwareKeyResponse"
QT_MOC_LITERAL(14, 181, 5) // "found"

    },
    "DatabaseOpenWidget\0dialogFinished\0\0"
    "accepted\0openDatabase\0reject\0browseKeyFile\0"
    "toggleHardwareKeyComponent\0state\0"
    "closeDatabase\0pollHardwareKey\0"
    "manualTrigger\0delay\0hardwareKeyResponse\0"
    "found"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DatabaseOpenWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   67,    2, 0x09 /* Protected */,
       5,    0,   68,    2, 0x09 /* Protected */,
       6,    0,   69,    2, 0x08 /* Private */,
       7,    1,   70,    2, 0x08 /* Private */,
       9,    0,   73,    2, 0x08 /* Private */,
      10,    2,   74,    2, 0x08 /* Private */,
      10,    1,   79,    2, 0x28 /* Private | MethodCloned */,
      10,    0,   82,    2, 0x28 /* Private | MethodCloned */,
      13,    1,   83,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::Int,   11,   12,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   14,

       0        // eod
};

void DatabaseOpenWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DatabaseOpenWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->dialogFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->openDatabase(); break;
        case 2: _t->reject(); break;
        case 3: { bool _r = _t->browseKeyFile();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->toggleHardwareKeyComponent((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->closeDatabase(); break;
        case 6: _t->pollHardwareKey((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 7: _t->pollHardwareKey((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->pollHardwareKey(); break;
        case 9: _t->hardwareKeyResponse((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DatabaseOpenWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DatabaseOpenWidget::dialogFinished)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DatabaseOpenWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<DialogyWidget::staticMetaObject>(),
    qt_meta_stringdata_DatabaseOpenWidget.data,
    qt_meta_data_DatabaseOpenWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DatabaseOpenWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DatabaseOpenWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DatabaseOpenWidget.stringdata0))
        return static_cast<void*>(this);
    return DialogyWidget::qt_metacast(_clname);
}

int DatabaseOpenWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DialogyWidget::qt_metacall(_c, _id, _a);
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
void DatabaseOpenWidget::dialogFinished(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
