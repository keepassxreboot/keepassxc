/****************************************************************************
** Meta object code from reading C++ file 'ReportsWidgetHibp.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/reports/ReportsWidgetHibp.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ReportsWidgetHibp.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ReportsWidgetHibp_t {
    QByteArrayData data[15];
    char stringdata0[215];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ReportsWidgetHibp_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ReportsWidgetHibp_t qt_meta_stringdata_ReportsWidgetHibp = {
    {
QT_MOC_LITERAL(0, 0, 17), // "ReportsWidgetHibp"
QT_MOC_LITERAL(1, 18, 14), // "entryActivated"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 6), // "Entry*"
QT_MOC_LITERAL(4, 41, 18), // "emitEntryActivated"
QT_MOC_LITERAL(5, 60, 11), // "QModelIndex"
QT_MOC_LITERAL(6, 72, 13), // "addHibpResult"
QT_MOC_LITERAL(7, 86, 11), // "fetchFailed"
QT_MOC_LITERAL(8, 98, 5), // "error"
QT_MOC_LITERAL(9, 104, 13), // "makeHibpTable"
QT_MOC_LITERAL(10, 118, 19), // "customMenuRequested"
QT_MOC_LITERAL(11, 138, 18), // "getSelectedEntries"
QT_MOC_LITERAL(12, 157, 13), // "QList<Entry*>"
QT_MOC_LITERAL(13, 171, 21), // "expireSelectedEntries"
QT_MOC_LITERAL(14, 193, 21) // "deleteSelectedEntries"

    },
    "ReportsWidgetHibp\0entryActivated\0\0"
    "Entry*\0emitEntryActivated\0QModelIndex\0"
    "addHibpResult\0fetchFailed\0error\0"
    "makeHibpTable\0customMenuRequested\0"
    "getSelectedEntries\0QList<Entry*>\0"
    "expireSelectedEntries\0deleteSelectedEntries"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ReportsWidgetHibp[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   62,    2, 0x0a /* Public */,
       6,    2,   65,    2, 0x0a /* Public */,
       7,    1,   70,    2, 0x0a /* Public */,
       9,    0,   73,    2, 0x0a /* Public */,
      10,    1,   74,    2, 0x0a /* Public */,
      11,    0,   77,    2, 0x0a /* Public */,
      13,    0,   78,    2, 0x0a /* Public */,
      14,    0,   79,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    2,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    2,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    2,    2,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,    2,
    0x80000000 | 12,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ReportsWidgetHibp::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ReportsWidgetHibp *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->entryActivated((*reinterpret_cast< Entry*(*)>(_a[1]))); break;
        case 1: _t->emitEntryActivated((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 2: _t->addHibpResult((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->fetchFailed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->makeHibpTable(); break;
        case 5: _t->customMenuRequested((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 6: { QList<Entry*> _r = _t->getSelectedEntries();
            if (_a[0]) *reinterpret_cast< QList<Entry*>*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->expireSelectedEntries(); break;
        case 8: _t->deleteSelectedEntries(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ReportsWidgetHibp::*)(Entry * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ReportsWidgetHibp::entryActivated)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ReportsWidgetHibp::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ReportsWidgetHibp.data,
    qt_meta_data_ReportsWidgetHibp,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ReportsWidgetHibp::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ReportsWidgetHibp::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ReportsWidgetHibp.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ReportsWidgetHibp::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void ReportsWidgetHibp::entryActivated(Entry * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
