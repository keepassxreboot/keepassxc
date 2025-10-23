/****************************************************************************
** Meta object code from reading C++ file 'EditGroupWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/group/EditGroupWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditGroupWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EditGroupWidget_t {
    QByteArrayData data[10];
    char stringdata0[125];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditGroupWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditGroupWidget_t qt_meta_stringdata_EditGroupWidget = {
    {
QT_MOC_LITERAL(0, 0, 15), // "EditGroupWidget"
QT_MOC_LITERAL(1, 16, 12), // "editFinished"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 8), // "accepted"
QT_MOC_LITERAL(4, 39, 16), // "messageEditEntry"
QT_MOC_LITERAL(5, 56, 26), // "MessageWidget::MessageType"
QT_MOC_LITERAL(6, 83, 23), // "messageEditEntryDismiss"
QT_MOC_LITERAL(7, 107, 5), // "apply"
QT_MOC_LITERAL(8, 113, 4), // "save"
QT_MOC_LITERAL(9, 118, 6) // "cancel"

    },
    "EditGroupWidget\0editFinished\0\0accepted\0"
    "messageEditEntry\0MessageWidget::MessageType\0"
    "messageEditEntryDismiss\0apply\0save\0"
    "cancel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditGroupWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       4,    2,   47,    2, 0x06 /* Public */,
       6,    0,   52,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   53,    2, 0x08 /* Private */,
       8,    0,   54,    2, 0x08 /* Private */,
       9,    0,   55,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 5,    2,    2,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EditGroupWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EditGroupWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->editFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->messageEditEntry((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2]))); break;
        case 2: _t->messageEditEntryDismiss(); break;
        case 3: _t->apply(); break;
        case 4: _t->save(); break;
        case 5: _t->cancel(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EditGroupWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EditGroupWidget::editFinished)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EditGroupWidget::*)(QString , MessageWidget::MessageType );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EditGroupWidget::messageEditEntry)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EditGroupWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EditGroupWidget::messageEditEntryDismiss)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject EditGroupWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<EditWidget::staticMetaObject>(),
    qt_meta_stringdata_EditGroupWidget.data,
    qt_meta_data_EditGroupWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EditGroupWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditGroupWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EditGroupWidget.stringdata0))
        return static_cast<void*>(this);
    return EditWidget::qt_metacast(_clname);
}

int EditGroupWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = EditWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void EditGroupWidget::editFinished(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EditGroupWidget::messageEditEntry(QString _t1, MessageWidget::MessageType _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EditGroupWidget::messageEditEntryDismiss()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
