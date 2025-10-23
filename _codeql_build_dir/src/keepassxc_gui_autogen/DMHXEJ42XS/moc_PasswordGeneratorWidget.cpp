/****************************************************************************
** Meta object code from reading C++ file 'PasswordGeneratorWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/PasswordGeneratorWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PasswordGeneratorWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PasswordGeneratorWidget_t {
    QByteArrayData data[22];
    char stringdata0[343];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PasswordGeneratorWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PasswordGeneratorWidget_t qt_meta_stringdata_PasswordGeneratorWidget = {
    {
QT_MOC_LITERAL(0, 0, 23), // "PasswordGeneratorWidget"
QT_MOC_LITERAL(1, 24, 15), // "appliedPassword"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 8), // "password"
QT_MOC_LITERAL(4, 50, 6), // "closed"
QT_MOC_LITERAL(5, 57, 18), // "regeneratePassword"
QT_MOC_LITERAL(6, 76, 13), // "applyPassword"
QT_MOC_LITERAL(7, 90, 12), // "copyPassword"
QT_MOC_LITERAL(8, 103, 18), // "setPasswordVisible"
QT_MOC_LITERAL(9, 122, 7), // "visible"
QT_MOC_LITERAL(10, 130, 20), // "removeCustomWordList"
QT_MOC_LITERAL(11, 151, 11), // "addWordList"
QT_MOC_LITERAL(12, 163, 20), // "updateButtonsEnabled"
QT_MOC_LITERAL(13, 184, 22), // "updatePasswordStrength"
QT_MOC_LITERAL(14, 207, 25), // "updatePasswordLengthLabel"
QT_MOC_LITERAL(15, 233, 15), // "setAdvancedMode"
QT_MOC_LITERAL(16, 249, 8), // "advanced"
QT_MOC_LITERAL(17, 258, 15), // "excludeHexChars"
QT_MOC_LITERAL(18, 274, 21), // "passwordLengthChanged"
QT_MOC_LITERAL(19, 296, 6), // "length"
QT_MOC_LITERAL(20, 303, 23), // "passphraseLengthChanged"
QT_MOC_LITERAL(21, 327, 15) // "updateGenerator"

    },
    "PasswordGeneratorWidget\0appliedPassword\0"
    "\0password\0closed\0regeneratePassword\0"
    "applyPassword\0copyPassword\0"
    "setPasswordVisible\0visible\0"
    "removeCustomWordList\0addWordList\0"
    "updateButtonsEnabled\0updatePasswordStrength\0"
    "updatePasswordLengthLabel\0setAdvancedMode\0"
    "advanced\0excludeHexChars\0passwordLengthChanged\0"
    "length\0passphraseLengthChanged\0"
    "updateGenerator"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PasswordGeneratorWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   94,    2, 0x06 /* Public */,
       4,    0,   97,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   98,    2, 0x0a /* Public */,
       6,    0,   99,    2, 0x0a /* Public */,
       7,    0,  100,    2, 0x0a /* Public */,
       8,    1,  101,    2, 0x0a /* Public */,
      10,    0,  104,    2, 0x0a /* Public */,
      11,    0,  105,    2, 0x0a /* Public */,
      12,    1,  106,    2, 0x08 /* Private */,
      13,    0,  109,    2, 0x08 /* Private */,
      14,    1,  110,    2, 0x08 /* Private */,
      15,    1,  113,    2, 0x08 /* Private */,
      17,    0,  116,    2, 0x08 /* Private */,
      18,    1,  117,    2, 0x08 /* Private */,
      20,    1,  120,    2, 0x08 /* Private */,
      21,    0,  123,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Bool,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void,

       0        // eod
};

void PasswordGeneratorWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PasswordGeneratorWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->appliedPassword((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->closed(); break;
        case 2: _t->regeneratePassword(); break;
        case 3: _t->applyPassword(); break;
        case 4: _t->copyPassword(); break;
        case 5: _t->setPasswordVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->removeCustomWordList(); break;
        case 7: _t->addWordList(); break;
        case 8: _t->updateButtonsEnabled((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->updatePasswordStrength(); break;
        case 10: _t->updatePasswordLengthLabel((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->setAdvancedMode((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->excludeHexChars(); break;
        case 13: _t->passwordLengthChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->passphraseLengthChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 15: _t->updateGenerator(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PasswordGeneratorWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PasswordGeneratorWidget::appliedPassword)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PasswordGeneratorWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PasswordGeneratorWidget::closed)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject PasswordGeneratorWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_PasswordGeneratorWidget.data,
    qt_meta_data_PasswordGeneratorWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PasswordGeneratorWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PasswordGeneratorWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PasswordGeneratorWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PasswordGeneratorWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void PasswordGeneratorWidget::appliedPassword(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PasswordGeneratorWidget::closed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
