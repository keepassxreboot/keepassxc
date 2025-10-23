/****************************************************************************
** Meta object code from reading C++ file 'GroupView.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/group/GroupView.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GroupView.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_GroupView_t {
    QByteArrayData data[15];
    char stringdata0[189];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GroupView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GroupView_t qt_meta_stringdata_GroupView = {
    {
QT_MOC_LITERAL(0, 0, 9), // "GroupView"
QT_MOC_LITERAL(1, 10, 21), // "groupSelectionChanged"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 12), // "groupFocused"
QT_MOC_LITERAL(4, 46, 15), // "expandedChanged"
QT_MOC_LITERAL(5, 62, 11), // "QModelIndex"
QT_MOC_LITERAL(6, 74, 5), // "index"
QT_MOC_LITERAL(7, 80, 17), // "syncExpandedState"
QT_MOC_LITERAL(8, 98, 6), // "parent"
QT_MOC_LITERAL(9, 105, 5), // "start"
QT_MOC_LITERAL(10, 111, 3), // "end"
QT_MOC_LITERAL(11, 115, 10), // "modelReset"
QT_MOC_LITERAL(12, 126, 26), // "contextMenuShortcutPressed"
QT_MOC_LITERAL(13, 153, 19), // "selectPreviousGroup"
QT_MOC_LITERAL(14, 173, 15) // "selectNextGroup"

    },
    "GroupView\0groupSelectionChanged\0\0"
    "groupFocused\0expandedChanged\0QModelIndex\0"
    "index\0syncExpandedState\0parent\0start\0"
    "end\0modelReset\0contextMenuShortcutPressed\0"
    "selectPreviousGroup\0selectNextGroup"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GroupView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   56,    2, 0x08 /* Private */,
       7,    3,   59,    2, 0x08 /* Private */,
      11,    0,   66,    2, 0x08 /* Private */,
      12,    0,   67,    2, 0x08 /* Private */,
      13,    0,   68,    2, 0x08 /* Private */,
      14,    0,   69,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5, QMetaType::Int, QMetaType::Int,    8,    9,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void GroupView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GroupView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->groupSelectionChanged(); break;
        case 1: _t->groupFocused(); break;
        case 2: _t->expandedChanged((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->syncExpandedState((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->modelReset(); break;
        case 5: _t->contextMenuShortcutPressed(); break;
        case 6: _t->selectPreviousGroup(); break;
        case 7: _t->selectNextGroup(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GroupView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GroupView::groupSelectionChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GroupView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GroupView::groupFocused)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject GroupView::staticMetaObject = { {
    QMetaObject::SuperData::link<QTreeView::staticMetaObject>(),
    qt_meta_stringdata_GroupView.data,
    qt_meta_data_GroupView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *GroupView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GroupView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GroupView.stringdata0))
        return static_cast<void*>(this);
    return QTreeView::qt_metacast(_clname);
}

int GroupView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeView::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void GroupView::groupSelectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GroupView::groupFocused()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
