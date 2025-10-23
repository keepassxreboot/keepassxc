/****************************************************************************
** Meta object code from reading C++ file 'ReportsWidgetHealthcheck.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/reports/ReportsWidgetHealthcheck.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ReportsWidgetHealthcheck.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ReportsWidgetHealthcheck_t {
    QByteArrayData data[13];
    char stringdata0[198];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ReportsWidgetHealthcheck_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ReportsWidgetHealthcheck_t qt_meta_stringdata_ReportsWidgetHealthcheck = {
    {
QT_MOC_LITERAL(0, 0, 24), // "ReportsWidgetHealthcheck"
QT_MOC_LITERAL(1, 25, 14), // "entryActivated"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 6), // "Entry*"
QT_MOC_LITERAL(4, 48, 15), // "calculateHealth"
QT_MOC_LITERAL(5, 64, 18), // "emitEntryActivated"
QT_MOC_LITERAL(6, 83, 11), // "QModelIndex"
QT_MOC_LITERAL(7, 95, 5), // "index"
QT_MOC_LITERAL(8, 101, 19), // "customMenuRequested"
QT_MOC_LITERAL(9, 121, 18), // "getSelectedEntries"
QT_MOC_LITERAL(10, 140, 13), // "QList<Entry*>"
QT_MOC_LITERAL(11, 154, 21), // "expireSelectedEntries"
QT_MOC_LITERAL(12, 176, 21) // "deleteSelectedEntries"

    },
    "ReportsWidgetHealthcheck\0entryActivated\0"
    "\0Entry*\0calculateHealth\0emitEntryActivated\0"
    "QModelIndex\0index\0customMenuRequested\0"
    "getSelectedEntries\0QList<Entry*>\0"
    "expireSelectedEntries\0deleteSelectedEntries"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ReportsWidgetHealthcheck[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   52,    2, 0x0a /* Public */,
       5,    1,   53,    2, 0x0a /* Public */,
       8,    1,   56,    2, 0x0a /* Public */,
       9,    0,   59,    2, 0x0a /* Public */,
      11,    0,   60,    2, 0x0a /* Public */,
      12,    0,   61,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    2,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, QMetaType::QPoint,    2,
    0x80000000 | 10,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ReportsWidgetHealthcheck::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ReportsWidgetHealthcheck *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->entryActivated((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 1: _t->calculateHealth(); break;
        case 2: _t->emitEntryActivated((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->customMenuRequested((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 4: { QList<Entry*> _r = _t->getSelectedEntries();
            if (_a[0]) *reinterpret_cast< QList<Entry*>*>(_a[0]) = std::move(_r); }  break;
        case 5: _t->expireSelectedEntries(); break;
        case 6: _t->deleteSelectedEntries(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ReportsWidgetHealthcheck::*)(Entry * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ReportsWidgetHealthcheck::entryActivated)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ReportsWidgetHealthcheck::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ReportsWidgetHealthcheck.data,
    qt_meta_data_ReportsWidgetHealthcheck,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ReportsWidgetHealthcheck::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ReportsWidgetHealthcheck::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ReportsWidgetHealthcheck.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ReportsWidgetHealthcheck::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void ReportsWidgetHealthcheck::entryActivated(Entry * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
