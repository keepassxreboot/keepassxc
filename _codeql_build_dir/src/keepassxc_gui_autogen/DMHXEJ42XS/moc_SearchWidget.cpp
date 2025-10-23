/****************************************************************************
** Meta object code from reading C++ file 'SearchWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/SearchWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SearchWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SearchWidget_t {
    QByteArrayData data[28];
    char stringdata0[391];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SearchWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SearchWidget_t qt_meta_stringdata_SearchWidget = {
    {
QT_MOC_LITERAL(0, 0, 12), // "SearchWidget"
QT_MOC_LITERAL(1, 13, 6), // "search"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 4), // "text"
QT_MOC_LITERAL(4, 26, 14), // "searchCanceled"
QT_MOC_LITERAL(5, 41, 20), // "caseSensitiveChanged"
QT_MOC_LITERAL(6, 62, 5), // "state"
QT_MOC_LITERAL(7, 68, 17), // "limitGroupChanged"
QT_MOC_LITERAL(8, 86, 13), // "escapePressed"
QT_MOC_LITERAL(9, 100, 11), // "downPressed"
QT_MOC_LITERAL(10, 112, 12), // "enterPressed"
QT_MOC_LITERAL(11, 125, 9), // "lostFocus"
QT_MOC_LITERAL(12, 135, 10), // "saveSearch"
QT_MOC_LITERAL(13, 146, 15), // "databaseChanged"
QT_MOC_LITERAL(14, 162, 15), // "DatabaseWidget*"
QT_MOC_LITERAL(15, 178, 8), // "dbWidget"
QT_MOC_LITERAL(16, 187, 11), // "focusSearch"
QT_MOC_LITERAL(17, 199, 11), // "clearSearch"
QT_MOC_LITERAL(18, 211, 15), // "onReturnPressed"
QT_MOC_LITERAL(19, 227, 16), // "startSearchTimer"
QT_MOC_LITERAL(20, 244, 11), // "startSearch"
QT_MOC_LITERAL(21, 256, 19), // "updateCaseSensitive"
QT_MOC_LITERAL(22, 276, 16), // "updateLimitGroup"
QT_MOC_LITERAL(23, 293, 10), // "toggleHelp"
QT_MOC_LITERAL(24, 304, 14), // "showSearchMenu"
QT_MOC_LITERAL(25, 319, 21), // "resetSearchClearTimer"
QT_MOC_LITERAL(26, 341, 22), // "performRequestedSearch"
QT_MOC_LITERAL(27, 364, 26) // "updateSaveButtonVisibility"

    },
    "SearchWidget\0search\0\0text\0searchCanceled\0"
    "caseSensitiveChanged\0state\0limitGroupChanged\0"
    "escapePressed\0downPressed\0enterPressed\0"
    "lostFocus\0saveSearch\0databaseChanged\0"
    "DatabaseWidget*\0dbWidget\0focusSearch\0"
    "clearSearch\0onReturnPressed\0"
    "startSearchTimer\0startSearch\0"
    "updateCaseSensitive\0updateLimitGroup\0"
    "toggleHelp\0showSearchMenu\0"
    "resetSearchClearTimer\0performRequestedSearch\0"
    "updateSaveButtonVisibility"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SearchWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  129,    2, 0x06 /* Public */,
       4,    0,  132,    2, 0x06 /* Public */,
       5,    1,  133,    2, 0x06 /* Public */,
       7,    1,  136,    2, 0x06 /* Public */,
       8,    0,  139,    2, 0x06 /* Public */,
       9,    0,  140,    2, 0x06 /* Public */,
      10,    0,  141,    2, 0x06 /* Public */,
      11,    0,  142,    2, 0x06 /* Public */,
      12,    1,  143,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    1,  146,    2, 0x0a /* Public */,
      13,    0,  149,    2, 0x2a /* Public | MethodCloned */,
      16,    0,  150,    2, 0x0a /* Public */,
      17,    0,  151,    2, 0x0a /* Public */,
      18,    0,  152,    2, 0x08 /* Private */,
      19,    0,  153,    2, 0x08 /* Private */,
      20,    0,  154,    2, 0x08 /* Private */,
      21,    0,  155,    2, 0x08 /* Private */,
      22,    0,  156,    2, 0x08 /* Private */,
      23,    0,  157,    2, 0x08 /* Private */,
      24,    0,  158,    2, 0x08 /* Private */,
      25,    0,  159,    2, 0x08 /* Private */,
      26,    1,  160,    2, 0x08 /* Private */,
      27,    0,  163,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

       0        // eod
};

void SearchWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SearchWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->search((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->searchCanceled(); break;
        case 2: _t->caseSensitiveChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->limitGroupChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->escapePressed(); break;
        case 5: _t->downPressed(); break;
        case 6: _t->enterPressed(); break;
        case 7: _t->lostFocus(); break;
        case 8: _t->saveSearch((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->databaseChanged((*reinterpret_cast< DatabaseWidget*(*)>(_a[1]))); break;
        case 10: _t->databaseChanged(); break;
        case 11: _t->focusSearch(); break;
        case 12: _t->clearSearch(); break;
        case 13: _t->onReturnPressed(); break;
        case 14: _t->startSearchTimer(); break;
        case 15: _t->startSearch(); break;
        case 16: _t->updateCaseSensitive(); break;
        case 17: _t->updateLimitGroup(); break;
        case 18: _t->toggleHelp(); break;
        case 19: _t->showSearchMenu(); break;
        case 20: _t->resetSearchClearTimer(); break;
        case 21: _t->performRequestedSearch((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 22: _t->updateSaveButtonVisibility(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< DatabaseWidget* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SearchWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::search)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::searchCanceled)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::caseSensitiveChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::limitGroupChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::escapePressed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::downPressed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::enterPressed)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::lostFocus)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SearchWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchWidget::saveSearch)) {
                *result = 8;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SearchWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SearchWidget.data,
    qt_meta_data_SearchWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SearchWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SearchWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SearchWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SearchWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    return _id;
}

// SIGNAL 0
void SearchWidget::search(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SearchWidget::searchCanceled()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SearchWidget::caseSensitiveChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SearchWidget::limitGroupChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SearchWidget::escapePressed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SearchWidget::downPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SearchWidget::enterPressed()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SearchWidget::lostFocus()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SearchWidget::saveSearch(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
