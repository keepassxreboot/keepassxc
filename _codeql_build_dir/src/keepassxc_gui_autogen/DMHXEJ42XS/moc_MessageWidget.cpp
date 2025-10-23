/****************************************************************************
** Meta object code from reading C++ file 'MessageWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/MessageWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MessageWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MessageWidget_t {
    QByteArrayData data[13];
    char stringdata0[169];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MessageWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MessageWidget_t qt_meta_stringdata_MessageWidget = {
    {
QT_MOC_LITERAL(0, 0, 13), // "MessageWidget"
QT_MOC_LITERAL(1, 14, 20), // "showAnimationStarted"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 20), // "hideAnimationStarted"
QT_MOC_LITERAL(4, 57, 11), // "showMessage"
QT_MOC_LITERAL(5, 69, 4), // "text"
QT_MOC_LITERAL(6, 74, 26), // "MessageWidget::MessageType"
QT_MOC_LITERAL(7, 101, 4), // "type"
QT_MOC_LITERAL(8, 106, 15), // "autoHideTimeout"
QT_MOC_LITERAL(9, 122, 11), // "hideMessage"
QT_MOC_LITERAL(10, 134, 18), // "setAutoHideTimeout"
QT_MOC_LITERAL(11, 153, 11), // "openHttpUrl"
QT_MOC_LITERAL(12, 165, 3) // "url"

    },
    "MessageWidget\0showAnimationStarted\0\0"
    "hideAnimationStarted\0showMessage\0text\0"
    "MessageWidget::MessageType\0type\0"
    "autoHideTimeout\0hideMessage\0"
    "setAutoHideTimeout\0openHttpUrl\0url"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MessageWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x06 /* Public */,
       3,    0,   50,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    2,   51,    2, 0x0a /* Public */,
       4,    3,   56,    2, 0x0a /* Public */,
       9,    0,   63,    2, 0x0a /* Public */,
      10,    1,   64,    2, 0x0a /* Public */,
      11,    1,   67,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 6,    5,    7,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 6, QMetaType::Int,    5,    7,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::QString,   12,

       0        // eod
};

void MessageWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MessageWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->showAnimationStarted(); break;
        case 1: _t->hideAnimationStarted(); break;
        case 2: _t->showMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2]))); break;
        case 3: _t->showMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< MessageWidget::MessageType(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->hideMessage(); break;
        case 5: _t->setAutoHideTimeout((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->openHttpUrl((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MessageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageWidget::showAnimationStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MessageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageWidget::hideAnimationStarted)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MessageWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<KMessageWidget::staticMetaObject>(),
    qt_meta_stringdata_MessageWidget.data,
    qt_meta_data_MessageWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MessageWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MessageWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MessageWidget.stringdata0))
        return static_cast<void*>(this);
    return KMessageWidget::qt_metacast(_clname);
}

int MessageWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = KMessageWidget::qt_metacall(_c, _id, _a);
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
void MessageWidget::showAnimationStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MessageWidget::hideAnimationStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
