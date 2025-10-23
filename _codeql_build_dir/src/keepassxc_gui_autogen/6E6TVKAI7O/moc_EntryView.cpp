/****************************************************************************
** Meta object code from reading C++ file 'EntryView.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/EntryView.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryView.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryView_t {
    QByteArrayData data[26];
    char stringdata0[370];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryView_t qt_meta_stringdata_EntryView = {
    {
QT_MOC_LITERAL(0, 0, 9), // "EntryView"
QT_MOC_LITERAL(1, 10, 14), // "entryActivated"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 6), // "Entry*"
QT_MOC_LITERAL(4, 33, 5), // "entry"
QT_MOC_LITERAL(5, 39, 23), // "EntryModel::ModelColumn"
QT_MOC_LITERAL(6, 63, 6), // "column"
QT_MOC_LITERAL(7, 70, 21), // "entrySelectionChanged"
QT_MOC_LITERAL(8, 92, 16), // "viewStateChanged"
QT_MOC_LITERAL(9, 109, 18), // "emitEntryActivated"
QT_MOC_LITERAL(10, 128, 11), // "QModelIndex"
QT_MOC_LITERAL(11, 140, 5), // "index"
QT_MOC_LITERAL(12, 146, 14), // "showHeaderMenu"
QT_MOC_LITERAL(13, 161, 8), // "position"
QT_MOC_LITERAL(14, 170, 22), // "toggleColumnVisibility"
QT_MOC_LITERAL(15, 193, 8), // "QAction*"
QT_MOC_LITERAL(16, 202, 6), // "action"
QT_MOC_LITERAL(17, 209, 18), // "fitColumnsToWindow"
QT_MOC_LITERAL(18, 228, 20), // "fitColumnsToContents"
QT_MOC_LITERAL(19, 249, 19), // "resetViewToDefaults"
QT_MOC_LITERAL(20, 269, 26), // "contextMenuShortcutPressed"
QT_MOC_LITERAL(21, 296, 20), // "sortIndicatorChanged"
QT_MOC_LITERAL(22, 317, 12), // "logicalIndex"
QT_MOC_LITERAL(23, 330, 13), // "Qt::SortOrder"
QT_MOC_LITERAL(24, 344, 5), // "order"
QT_MOC_LITERAL(25, 350, 19) // "jumpToGroupShortcut"

    },
    "EntryView\0entryActivated\0\0Entry*\0entry\0"
    "EntryModel::ModelColumn\0column\0"
    "entrySelectionChanged\0viewStateChanged\0"
    "emitEntryActivated\0QModelIndex\0index\0"
    "showHeaderMenu\0position\0toggleColumnVisibility\0"
    "QAction*\0action\0fitColumnsToWindow\0"
    "fitColumnsToContents\0resetViewToDefaults\0"
    "contextMenuShortcutPressed\0"
    "sortIndicatorChanged\0logicalIndex\0"
    "Qt::SortOrder\0order\0jumpToGroupShortcut"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   74,    2, 0x06 /* Public */,
       7,    1,   79,    2, 0x06 /* Public */,
       8,    0,   82,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    1,   83,    2, 0x08 /* Private */,
      12,    1,   86,    2, 0x08 /* Private */,
      14,    1,   89,    2, 0x08 /* Private */,
      17,    0,   92,    2, 0x08 /* Private */,
      18,    0,   93,    2, 0x08 /* Private */,
      19,    0,   94,    2, 0x08 /* Private */,
      20,    0,   95,    2, 0x08 /* Private */,
      21,    2,   96,    2, 0x08 /* Private */,
      25,    0,  101,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, QMetaType::QPoint,   13,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 23,   22,   24,
    QMetaType::Void,

       0        // eod
};

void EntryView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->entryActivated((*reinterpret_cast< Entry*(*)>(_a[1])),(*reinterpret_cast< EntryModel::ModelColumn(*)>(_a[2]))); break;
        case 1: _t->entrySelectionChanged((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 2: _t->viewStateChanged(); break;
        case 3: _t->emitEntryActivated((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 4: _t->showHeaderMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 5: _t->toggleColumnVisibility((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 6: _t->fitColumnsToWindow(); break;
        case 7: _t->fitColumnsToContents(); break;
        case 8: _t->resetViewToDefaults(); break;
        case 9: _t->contextMenuShortcutPressed(); break;
        case 10: _t->sortIndicatorChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< Qt::SortOrder(*)>(_a[2]))); break;
        case 11: _t->jumpToGroupShortcut(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EntryView::*)(Entry * , EntryModel::ModelColumn );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryView::entryActivated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EntryView::*)(Entry * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryView::entrySelectionChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EntryView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryView::viewStateChanged)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EntryView::staticMetaObject = { {
    QMetaObject::SuperData::link<QTreeView::staticMetaObject>(),
    qt_meta_stringdata_EntryView.data,
    qt_meta_data_EntryView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryView.stringdata0))
        return static_cast<void*>(this);
    return QTreeView::qt_metacast(_clname);
}

int EntryView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTreeView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void EntryView::entryActivated(Entry * _t1, EntryModel::ModelColumn _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EntryView::entrySelectionChanged(Entry * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EntryView::viewStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
