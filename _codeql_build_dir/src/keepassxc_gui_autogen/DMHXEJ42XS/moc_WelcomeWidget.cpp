/****************************************************************************
** Meta object code from reading C++ file 'WelcomeWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/WelcomeWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WelcomeWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_WelcomeWidget_t {
    QByteArrayData data[9];
    char stringdata0[111];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WelcomeWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WelcomeWidget_t qt_meta_stringdata_WelcomeWidget = {
    {
QT_MOC_LITERAL(0, 0, 13), // "WelcomeWidget"
QT_MOC_LITERAL(1, 14, 11), // "newDatabase"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 12), // "openDatabase"
QT_MOC_LITERAL(4, 40, 16), // "openDatabaseFile"
QT_MOC_LITERAL(5, 57, 10), // "importFile"
QT_MOC_LITERAL(6, 68, 20), // "openDatabaseFromFile"
QT_MOC_LITERAL(7, 89, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(8, 106, 4) // "item"

    },
    "WelcomeWidget\0newDatabase\0\0openDatabase\0"
    "openDatabaseFile\0importFile\0"
    "openDatabaseFromFile\0QListWidgetItem*\0"
    "item"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WelcomeWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x06 /* Public */,
       3,    0,   40,    2, 0x06 /* Public */,
       4,    1,   41,    2, 0x06 /* Public */,
       5,    0,   44,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    1,   45,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

void WelcomeWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WelcomeWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->newDatabase(); break;
        case 1: _t->openDatabase(); break;
        case 2: _t->openDatabaseFile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->importFile(); break;
        case 4: _t->openDatabaseFromFile((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (WelcomeWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WelcomeWidget::newDatabase)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (WelcomeWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WelcomeWidget::openDatabase)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (WelcomeWidget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WelcomeWidget::openDatabaseFile)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (WelcomeWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WelcomeWidget::importFile)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject WelcomeWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_WelcomeWidget.data,
    qt_meta_data_WelcomeWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *WelcomeWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WelcomeWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WelcomeWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int WelcomeWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void WelcomeWidget::newDatabase()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void WelcomeWidget::openDatabase()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void WelcomeWidget::openDatabaseFile(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void WelcomeWidget::importFile()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
