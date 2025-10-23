/****************************************************************************
** Meta object code from reading C++ file 'DatabaseSettingsWidgetMaintenance.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/dbsettings/DatabaseSettingsWidgetMaintenance.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatabaseSettingsWidgetMaintenance.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DatabaseSettingsWidgetMaintenance_t {
    QByteArrayData data[8];
    char stringdata0[129];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DatabaseSettingsWidgetMaintenance_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DatabaseSettingsWidgetMaintenance_t qt_meta_stringdata_DatabaseSettingsWidgetMaintenance = {
    {
QT_MOC_LITERAL(0, 0, 33), // "DatabaseSettingsWidgetMainten..."
QT_MOC_LITERAL(1, 34, 10), // "initialize"
QT_MOC_LITERAL(2, 45, 0), // ""
QT_MOC_LITERAL(3, 46, 12), // "uninitialize"
QT_MOC_LITERAL(4, 59, 12), // "saveSettings"
QT_MOC_LITERAL(5, 72, 16), // "selectionChanged"
QT_MOC_LITERAL(6, 89, 16), // "removeCustomIcon"
QT_MOC_LITERAL(7, 106, 22) // "purgeUnusedCustomIcons"

    },
    "DatabaseSettingsWidgetMaintenance\0"
    "initialize\0\0uninitialize\0saveSettings\0"
    "selectionChanged\0removeCustomIcon\0"
    "purgeUnusedCustomIcons"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DatabaseSettingsWidgetMaintenance[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x0a /* Public */,
       3,    0,   45,    2, 0x0a /* Public */,
       4,    0,   46,    2, 0x0a /* Public */,
       5,    0,   47,    2, 0x08 /* Private */,
       6,    0,   48,    2, 0x08 /* Private */,
       7,    0,   49,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DatabaseSettingsWidgetMaintenance::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DatabaseSettingsWidgetMaintenance *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->initialize(); break;
        case 1: _t->uninitialize(); break;
        case 2: { bool _r = _t->saveSettings();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->selectionChanged(); break;
        case 4: _t->removeCustomIcon(); break;
        case 5: _t->purgeUnusedCustomIcons(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DatabaseSettingsWidgetMaintenance::staticMetaObject = { {
    QMetaObject::SuperData::link<DatabaseSettingsWidget::staticMetaObject>(),
    qt_meta_stringdata_DatabaseSettingsWidgetMaintenance.data,
    qt_meta_data_DatabaseSettingsWidgetMaintenance,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DatabaseSettingsWidgetMaintenance::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DatabaseSettingsWidgetMaintenance::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DatabaseSettingsWidgetMaintenance.stringdata0))
        return static_cast<void*>(this);
    return DatabaseSettingsWidget::qt_metacast(_clname);
}

int DatabaseSettingsWidgetMaintenance::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DatabaseSettingsWidget::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
