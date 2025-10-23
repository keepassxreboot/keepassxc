/****************************************************************************
** Meta object code from reading C++ file 'DatabaseSettingsWidgetGeneral.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/dbsettings/DatabaseSettingsWidgetGeneral.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatabaseSettingsWidgetGeneral.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DatabaseSettingsWidgetGeneral_t {
    QByteArrayData data[11];
    char stringdata0[160];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DatabaseSettingsWidgetGeneral_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DatabaseSettingsWidgetGeneral_t qt_meta_stringdata_DatabaseSettingsWidgetGeneral = {
    {
QT_MOC_LITERAL(0, 0, 29), // "DatabaseSettingsWidgetGeneral"
QT_MOC_LITERAL(1, 30, 10), // "initialize"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 12), // "uninitialize"
QT_MOC_LITERAL(4, 55, 12), // "saveSettings"
QT_MOC_LITERAL(5, 68, 15), // "pickPublicColor"
QT_MOC_LITERAL(6, 84, 22), // "setupPublicColorButton"
QT_MOC_LITERAL(7, 107, 5), // "color"
QT_MOC_LITERAL(8, 113, 14), // "pickPublicIcon"
QT_MOC_LITERAL(9, 128, 21), // "setupPublicIconButton"
QT_MOC_LITERAL(10, 150, 9) // "iconIndex"

    },
    "DatabaseSettingsWidgetGeneral\0initialize\0"
    "\0uninitialize\0saveSettings\0pickPublicColor\0"
    "setupPublicColorButton\0color\0"
    "pickPublicIcon\0setupPublicIconButton\0"
    "iconIndex"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DatabaseSettingsWidgetGeneral[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x0a /* Public */,
       3,    0,   50,    2, 0x0a /* Public */,
       4,    0,   51,    2, 0x0a /* Public */,
       5,    0,   52,    2, 0x08 /* Private */,
       6,    1,   53,    2, 0x08 /* Private */,
       8,    0,   56,    2, 0x08 /* Private */,
       9,    1,   57,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QColor,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,

       0        // eod
};

void DatabaseSettingsWidgetGeneral::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DatabaseSettingsWidgetGeneral *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->initialize(); break;
        case 1: _t->uninitialize(); break;
        case 2: { bool _r = _t->saveSettings();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->pickPublicColor(); break;
        case 4: _t->setupPublicColorButton((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 5: _t->pickPublicIcon(); break;
        case 6: _t->setupPublicIconButton((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DatabaseSettingsWidgetGeneral::staticMetaObject = { {
    QMetaObject::SuperData::link<DatabaseSettingsWidget::staticMetaObject>(),
    qt_meta_stringdata_DatabaseSettingsWidgetGeneral.data,
    qt_meta_data_DatabaseSettingsWidgetGeneral,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DatabaseSettingsWidgetGeneral::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DatabaseSettingsWidgetGeneral::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DatabaseSettingsWidgetGeneral.stringdata0))
        return static_cast<void*>(this);
    return DatabaseSettingsWidget::qt_metacast(_clname);
}

int DatabaseSettingsWidgetGeneral::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DatabaseSettingsWidget::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
