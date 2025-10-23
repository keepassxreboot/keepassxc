/****************************************************************************
** Meta object code from reading C++ file 'ElidedLabel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/widgets/ElidedLabel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ElidedLabel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ElidedLabel_t {
    QByteArrayData data[14];
    char stringdata0[150];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ElidedLabel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ElidedLabel_t qt_meta_stringdata_ElidedLabel = {
    {
QT_MOC_LITERAL(0, 0, 11), // "ElidedLabel"
QT_MOC_LITERAL(1, 12, 16), // "elideModeChanged"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 17), // "Qt::TextElideMode"
QT_MOC_LITERAL(4, 48, 9), // "elideMode"
QT_MOC_LITERAL(5, 58, 14), // "rawTextChanged"
QT_MOC_LITERAL(6, 73, 7), // "rawText"
QT_MOC_LITERAL(7, 81, 10), // "urlChanged"
QT_MOC_LITERAL(8, 92, 3), // "url"
QT_MOC_LITERAL(9, 96, 12), // "setElideMode"
QT_MOC_LITERAL(10, 109, 10), // "setRawText"
QT_MOC_LITERAL(11, 120, 6), // "setUrl"
QT_MOC_LITERAL(12, 127, 5), // "clear"
QT_MOC_LITERAL(13, 133, 16) // "updateElidedText"

    },
    "ElidedLabel\0elideModeChanged\0\0"
    "Qt::TextElideMode\0elideMode\0rawTextChanged\0"
    "rawText\0urlChanged\0url\0setElideMode\0"
    "setRawText\0setUrl\0clear\0updateElidedText"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ElidedLabel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       3,   74, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       5,    1,   57,    2, 0x06 /* Public */,
       7,    1,   60,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    1,   63,    2, 0x0a /* Public */,
      10,    1,   66,    2, 0x0a /* Public */,
      11,    1,   69,    2, 0x0a /* Public */,
      12,    0,   72,    2, 0x0a /* Public */,
      13,    0,   73,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    8,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
       4, 0x80000000 | 3, 0x0049510b,
       6, QMetaType::QString, 0x00495103,
       8, QMetaType::QString, 0x00495103,

 // properties: notify_signal_id
       0,
       1,
       2,

       0        // eod
};

void ElidedLabel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ElidedLabel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->elideModeChanged((*reinterpret_cast< Qt::TextElideMode(*)>(_a[1]))); break;
        case 1: _t->rawTextChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->urlChanged((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 3: _t->setElideMode((*reinterpret_cast< Qt::TextElideMode(*)>(_a[1]))); break;
        case 4: _t->setRawText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->setUrl((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->clear(); break;
        case 7: _t->updateElidedText(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ElidedLabel::*)(Qt::TextElideMode );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ElidedLabel::elideModeChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ElidedLabel::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ElidedLabel::rawTextChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ElidedLabel::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ElidedLabel::urlChanged)) {
                *result = 2;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<ElidedLabel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< Qt::TextElideMode*>(_v) = _t->elideMode(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->rawText(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->url(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<ElidedLabel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setElideMode(*reinterpret_cast< Qt::TextElideMode*>(_v)); break;
        case 1: _t->setRawText(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setUrl(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject ElidedLabel::staticMetaObject = { {
    QMetaObject::SuperData::link<QLabel::staticMetaObject>(),
    qt_meta_stringdata_ElidedLabel.data,
    qt_meta_data_ElidedLabel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ElidedLabel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ElidedLabel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ElidedLabel.stringdata0))
        return static_cast<void*>(this);
    return QLabel::qt_metacast(_clname);
}

int ElidedLabel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLabel::qt_metacall(_c, _id, _a);
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
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void ElidedLabel::elideModeChanged(Qt::TextElideMode _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ElidedLabel::rawTextChanged(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ElidedLabel::urlChanged(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
