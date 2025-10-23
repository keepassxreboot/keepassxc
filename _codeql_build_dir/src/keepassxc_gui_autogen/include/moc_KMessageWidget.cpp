/****************************************************************************
** Meta object code from reading C++ file 'KMessageWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/KMessageWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'KMessageWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_KMessageWidget_t {
    QByteArrayData data[29];
    char stringdata0[365];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_KMessageWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_KMessageWidget_t qt_meta_stringdata_KMessageWidget = {
    {
QT_MOC_LITERAL(0, 0, 14), // "KMessageWidget"
QT_MOC_LITERAL(1, 15, 13), // "linkActivated"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 8), // "contents"
QT_MOC_LITERAL(4, 39, 11), // "linkHovered"
QT_MOC_LITERAL(5, 51, 21), // "hideAnimationFinished"
QT_MOC_LITERAL(6, 73, 21), // "showAnimationFinished"
QT_MOC_LITERAL(7, 95, 7), // "setText"
QT_MOC_LITERAL(8, 103, 4), // "text"
QT_MOC_LITERAL(9, 108, 11), // "setWordWrap"
QT_MOC_LITERAL(10, 120, 8), // "wordWrap"
QT_MOC_LITERAL(11, 129, 21), // "setCloseButtonVisible"
QT_MOC_LITERAL(12, 151, 7), // "visible"
QT_MOC_LITERAL(13, 159, 14), // "setMessageType"
QT_MOC_LITERAL(14, 174, 27), // "KMessageWidget::MessageType"
QT_MOC_LITERAL(15, 202, 4), // "type"
QT_MOC_LITERAL(16, 207, 12), // "animatedShow"
QT_MOC_LITERAL(17, 220, 12), // "animatedHide"
QT_MOC_LITERAL(18, 233, 7), // "setIcon"
QT_MOC_LITERAL(19, 241, 4), // "icon"
QT_MOC_LITERAL(20, 246, 19), // "slotTimeLineChanged"
QT_MOC_LITERAL(21, 266, 20), // "slotTimeLineFinished"
QT_MOC_LITERAL(22, 287, 18), // "closeButtonVisible"
QT_MOC_LITERAL(23, 306, 11), // "messageType"
QT_MOC_LITERAL(24, 318, 11), // "MessageType"
QT_MOC_LITERAL(25, 330, 8), // "Positive"
QT_MOC_LITERAL(26, 339, 11), // "Information"
QT_MOC_LITERAL(27, 351, 7), // "Warning"
QT_MOC_LITERAL(28, 359, 5) // "Error"

    },
    "KMessageWidget\0linkActivated\0\0contents\0"
    "linkHovered\0hideAnimationFinished\0"
    "showAnimationFinished\0setText\0text\0"
    "setWordWrap\0wordWrap\0setCloseButtonVisible\0"
    "visible\0setMessageType\0"
    "KMessageWidget::MessageType\0type\0"
    "animatedShow\0animatedHide\0setIcon\0"
    "icon\0slotTimeLineChanged\0slotTimeLineFinished\0"
    "closeButtonVisible\0messageType\0"
    "MessageType\0Positive\0Information\0"
    "Warning\0Error"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_KMessageWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       5,  108, // properties
       1,  123, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x06 /* Public */,
       4,    1,   82,    2, 0x06 /* Public */,
       5,    0,   85,    2, 0x06 /* Public */,
       6,    0,   86,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   87,    2, 0x0a /* Public */,
       9,    1,   90,    2, 0x0a /* Public */,
      11,    1,   93,    2, 0x0a /* Public */,
      13,    1,   96,    2, 0x0a /* Public */,
      16,    0,   99,    2, 0x0a /* Public */,
      17,    0,  100,    2, 0x0a /* Public */,
      18,    1,  101,    2, 0x0a /* Public */,
      20,    1,  104,    2, 0x08 /* Private */,
      21,    0,  107,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QIcon,   19,
    QMetaType::Void, QMetaType::QReal,    2,
    QMetaType::Void,

 // properties: name, type, flags
       8, QMetaType::QString, 0x00095103,
      10, QMetaType::Bool, 0x00095103,
      22, QMetaType::Bool, 0x00095103,
      23, 0x80000000 | 24, 0x0009510b,
      19, QMetaType::QIcon, 0x00095103,

 // enums: name, alias, flags, count, data
      24,   24, 0x0,    4,  128,

 // enum data: key, value
      25, uint(KMessageWidget::Positive),
      26, uint(KMessageWidget::Information),
      27, uint(KMessageWidget::Warning),
      28, uint(KMessageWidget::Error),

       0        // eod
};

void KMessageWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<KMessageWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->linkActivated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->linkHovered((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->hideAnimationFinished(); break;
        case 3: _t->showAnimationFinished(); break;
        case 4: _t->setText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->setWordWrap((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->setCloseButtonVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->setMessageType((*reinterpret_cast< KMessageWidget::MessageType(*)>(_a[1]))); break;
        case 8: _t->animatedShow(); break;
        case 9: _t->animatedHide(); break;
        case 10: _t->setIcon((*reinterpret_cast< const QIcon(*)>(_a[1]))); break;
        case 11: _t->d->slotTimeLineChanged((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 12: _t->d->slotTimeLineFinished(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (KMessageWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KMessageWidget::linkActivated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (KMessageWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KMessageWidget::linkHovered)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (KMessageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KMessageWidget::hideAnimationFinished)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (KMessageWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&KMessageWidget::showAnimationFinished)) {
                *result = 3;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<KMessageWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->text(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->wordWrap(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->isCloseButtonVisible(); break;
        case 3: *reinterpret_cast< MessageType*>(_v) = _t->messageType(); break;
        case 4: *reinterpret_cast< QIcon*>(_v) = _t->icon(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<KMessageWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setText(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setWordWrap(*reinterpret_cast< bool*>(_v)); break;
        case 2: _t->setCloseButtonVisible(*reinterpret_cast< bool*>(_v)); break;
        case 3: _t->setMessageType(*reinterpret_cast< MessageType*>(_v)); break;
        case 4: _t->setIcon(*reinterpret_cast< QIcon*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject KMessageWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_KMessageWidget.data,
    qt_meta_data_KMessageWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *KMessageWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KMessageWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_KMessageWidget.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int KMessageWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 5;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void KMessageWidget::linkActivated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void KMessageWidget::linkHovered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void KMessageWidget::hideAnimationFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void KMessageWidget::showAnimationFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
