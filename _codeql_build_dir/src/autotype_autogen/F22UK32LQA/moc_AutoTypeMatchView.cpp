/****************************************************************************
** Meta object code from reading C++ file 'AutoTypeMatchView.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/autotype/AutoTypeMatchView.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AutoTypeMatchView.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AutoTypeMatchView_t {
    QByteArrayData data[10];
    char stringdata0[118];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AutoTypeMatchView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AutoTypeMatchView_t qt_meta_stringdata_AutoTypeMatchView = {
    {
QT_MOC_LITERAL(0, 0, 17), // "AutoTypeMatchView"
QT_MOC_LITERAL(1, 18, 19), // "currentMatchChanged"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 13), // "AutoTypeMatch"
QT_MOC_LITERAL(4, 53, 5), // "match"
QT_MOC_LITERAL(5, 59, 14), // "matchActivated"
QT_MOC_LITERAL(6, 74, 14), // "currentChanged"
QT_MOC_LITERAL(7, 89, 11), // "QModelIndex"
QT_MOC_LITERAL(8, 101, 7), // "current"
QT_MOC_LITERAL(9, 109, 8) // "previous"

    },
    "AutoTypeMatchView\0currentMatchChanged\0"
    "\0AutoTypeMatch\0match\0matchActivated\0"
    "currentChanged\0QModelIndex\0current\0"
    "previous"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AutoTypeMatchView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       5,    1,   32,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    2,   35,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 7,    8,    9,

       0        // eod
};

void AutoTypeMatchView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AutoTypeMatchView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->currentMatchChanged((*reinterpret_cast< AutoTypeMatch(*)>(_a[1]))); break;
        case 1: _t->matchActivated((*reinterpret_cast< AutoTypeMatch(*)>(_a[1]))); break;
        case 2: _t->currentChanged((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< const QModelIndex(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AutoTypeMatchView::*)(AutoTypeMatch );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeMatchView::currentMatchChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AutoTypeMatchView::*)(AutoTypeMatch );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AutoTypeMatchView::matchActivated)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AutoTypeMatchView::staticMetaObject = { {
    QMetaObject::SuperData::link<QTableView::staticMetaObject>(),
    qt_meta_stringdata_AutoTypeMatchView.data,
    qt_meta_data_AutoTypeMatchView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AutoTypeMatchView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AutoTypeMatchView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AutoTypeMatchView.stringdata0))
        return static_cast<void*>(this);
    return QTableView::qt_metacast(_clname);
}

int AutoTypeMatchView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTableView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void AutoTypeMatchView::currentMatchChanged(AutoTypeMatch _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AutoTypeMatchView::matchActivated(AutoTypeMatch _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
