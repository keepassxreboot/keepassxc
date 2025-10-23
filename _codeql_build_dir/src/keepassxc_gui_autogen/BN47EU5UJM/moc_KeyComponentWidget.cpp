/****************************************************************************
** Meta object code from reading C++ file 'KeyComponentWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/databasekey/KeyComponentWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'KeyComponentWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_KeyComponentWidget_t {
    QByteArrayData data[16];
    char stringdata0[230];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_KeyComponentWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_KeyComponentWidget_t qt_meta_stringdata_KeyComponentWidget = {
    {
QT_MOC_LITERAL(0, 0, 18), // "KeyComponentWidget"
QT_MOC_LITERAL(1, 19, 19), // "componentAddChanged"
QT_MOC_LITERAL(2, 39, 0), // ""
QT_MOC_LITERAL(3, 40, 5), // "added"
QT_MOC_LITERAL(4, 46, 21), // "componentAddRequested"
QT_MOC_LITERAL(5, 68, 22), // "componentEditRequested"
QT_MOC_LITERAL(6, 91, 12), // "editCanceled"
QT_MOC_LITERAL(7, 104, 25), // "componentRemovalRequested"
QT_MOC_LITERAL(8, 130, 15), // "updateAddStatus"
QT_MOC_LITERAL(9, 146, 5), // "doAdd"
QT_MOC_LITERAL(10, 152, 6), // "doEdit"
QT_MOC_LITERAL(11, 159, 8), // "doRemove"
QT_MOC_LITERAL(12, 168, 10), // "cancelEdit"
QT_MOC_LITERAL(13, 179, 24), // "resetComponentEditWidget"
QT_MOC_LITERAL(14, 204, 10), // "updateSize"
QT_MOC_LITERAL(15, 215, 14) // "componentAdded"

    },
    "KeyComponentWidget\0componentAddChanged\0"
    "\0added\0componentAddRequested\0"
    "componentEditRequested\0editCanceled\0"
    "componentRemovalRequested\0updateAddStatus\0"
    "doAdd\0doEdit\0doRemove\0cancelEdit\0"
    "resetComponentEditWidget\0updateSize\0"
    "componentAdded"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_KeyComponentWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       1,   90, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   74,    2, 0x06 /* Public */,
       4,    0,   77,    2, 0x06 /* Public */,
       5,    0,   78,    2, 0x06 /* Public */,
       6,    0,   79,    2, 0x06 /* Public */,
       7,    0,   80,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    1,   81,    2, 0x08 /* Private */,
       9,    0,   84,    2, 0x08 /* Private */,
      10,    0,   85,    2, 0x08 /* Private */,
      11,    0,   86,    2, 0x08 /* Private */,
      12,    0,   87,    2, 0x08 /* Private */,
      13,    0,   88,    2, 0x08 /* Private */,
      14,    0,   89,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      15, QMetaType::Bool, 0x00495103,

 // properties: notify_signal_id
       0,

       0        // eod
};

void KeyComponentWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<KeyComponentWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->componentAddChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->componentAddRequested(); break;
        case 2: _t->componentEditRequested(); break;
        case 3: _t->editCanceled(); break;
        case 4: _t->componentRemovalRequested(); break;
        case 5: _t->updateAddStatus((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->doAdd(); break;
        case 7: _t->doEdit(); break;
        case 8: _t->doRemove(); break;
        case 9: _t->cancelEdit(); break;
        case 10: _t->resetComponentEditWidget(); break;
        case 11: _t->updateSize(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (KeyComponentWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KeyComponentWidget::componentAddChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (KeyComponentWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KeyComponentWidget::componentAddRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (KeyComponentWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KeyComponentWidget::componentEditRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (KeyComponentWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KeyComponentWidget::editCanceled)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (KeyComponentWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KeyComponentWidget::componentRemovalRequested)) {
                *result = 4;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<KeyComponentWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->componentAdded(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<KeyComponentWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setComponentAdded(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject KeyComponentWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_KeyComponentWidget.data,
    qt_meta_data_KeyComponentWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *KeyComponentWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KeyComponentWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_KeyComponentWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int KeyComponentWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void KeyComponentWidget::componentAddChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void KeyComponentWidget::componentAddRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void KeyComponentWidget::componentEditRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void KeyComponentWidget::editCanceled()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void KeyComponentWidget::componentRemovalRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
