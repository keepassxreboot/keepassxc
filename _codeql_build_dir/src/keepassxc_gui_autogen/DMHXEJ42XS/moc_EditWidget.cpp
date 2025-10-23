/****************************************************************************
** Meta object code from reading C++ file 'EditWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/EditWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EditWidget_t {
    QByteArrayData data[15];
    char stringdata0[153];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditWidget_t qt_meta_stringdata_EditWidget = {
    {
QT_MOC_LITERAL(0, 0, 10), // "EditWidget"
QT_MOC_LITERAL(1, 11, 5), // "apply"
QT_MOC_LITERAL(2, 17, 0), // ""
QT_MOC_LITERAL(3, 18, 8), // "accepted"
QT_MOC_LITERAL(4, 27, 8), // "rejected"
QT_MOC_LITERAL(5, 36, 11), // "showMessage"
QT_MOC_LITERAL(6, 48, 4), // "text"
QT_MOC_LITERAL(7, 53, 26), // "MessageWidget::MessageType"
QT_MOC_LITERAL(8, 80, 4), // "type"
QT_MOC_LITERAL(9, 85, 11), // "hideMessage"
QT_MOC_LITERAL(10, 97, 11), // "setModified"
QT_MOC_LITERAL(11, 109, 5), // "state"
QT_MOC_LITERAL(12, 115, 13), // "buttonClicked"
QT_MOC_LITERAL(13, 129, 16), // "QAbstractButton*"
QT_MOC_LITERAL(14, 146, 6) // "button"

    },
    "EditWidget\0apply\0\0accepted\0rejected\0"
    "showMessage\0text\0MessageWidget::MessageType\0"
    "type\0hideMessage\0setModified\0state\0"
    "buttonClicked\0QAbstractButton*\0button"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    0,   56,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    2,   57,    2, 0x09 /* Protected */,
       9,    0,   62,    2, 0x09 /* Protected */,
      10,    1,   63,    2, 0x09 /* Protected */,
      10,    0,   66,    2, 0x29 /* Protected | MethodCloned */,
      12,    1,   67,    2, 0x09 /* Protected */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 7,    6,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 13,   14,

       0        // eod
};

void EditWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EditWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->apply(); break;
        case 1: _t->accepted(); break;
        case 2: _t->rejected(); break;
        case 3: _t->showMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2]))); break;
        case 4: _t->hideMessage(); break;
        case 5: _t->setModified((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->setModified(); break;
        case 7: _t->buttonClicked((*reinterpret_cast< QAbstractButton*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EditWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EditWidget::apply)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EditWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EditWidget::accepted)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EditWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EditWidget::rejected)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EditWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<DialogyWidget::staticMetaObject>(),
    qt_meta_stringdata_EditWidget.data,
    qt_meta_data_EditWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EditWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EditWidget.stringdata0))
        return static_cast<void*>(this);
    return DialogyWidget::qt_metacast(_clname);
}

int EditWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DialogyWidget::qt_metacall(_c, _id, _a);
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
void EditWidget::apply()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void EditWidget::accepted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void EditWidget::rejected()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
