/****************************************************************************
** Meta object code from reading C++ file 'Database.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/core/Database.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Database.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Database_t {
    QByteArrayData data[29];
    char stringdata0[380];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Database_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Database_t qt_meta_stringdata_Database = {
    {
QT_MOC_LITERAL(0, 0, 8), // "Database"
QT_MOC_LITERAL(1, 9, 15), // "filePathChanged"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 7), // "oldPath"
QT_MOC_LITERAL(4, 34, 7), // "newPath"
QT_MOC_LITERAL(5, 42, 16), // "groupDataChanged"
QT_MOC_LITERAL(6, 59, 6), // "Group*"
QT_MOC_LITERAL(7, 66, 5), // "group"
QT_MOC_LITERAL(8, 72, 15), // "groupAboutToAdd"
QT_MOC_LITERAL(9, 88, 5), // "index"
QT_MOC_LITERAL(10, 94, 10), // "groupAdded"
QT_MOC_LITERAL(11, 105, 18), // "groupAboutToRemove"
QT_MOC_LITERAL(12, 124, 12), // "groupRemoved"
QT_MOC_LITERAL(13, 137, 16), // "groupAboutToMove"
QT_MOC_LITERAL(14, 154, 7), // "toGroup"
QT_MOC_LITERAL(15, 162, 10), // "groupMoved"
QT_MOC_LITERAL(16, 173, 14), // "databaseOpened"
QT_MOC_LITERAL(17, 188, 13), // "databaseSaved"
QT_MOC_LITERAL(18, 202, 17), // "databaseDiscarded"
QT_MOC_LITERAL(19, 220, 19), // "databaseFileChanged"
QT_MOC_LITERAL(20, 240, 15), // "triggeredBySave"
QT_MOC_LITERAL(21, 256, 22), // "databaseNonDataChanged"
QT_MOC_LITERAL(22, 279, 14), // "tagListUpdated"
QT_MOC_LITERAL(23, 294, 14), // "markAsModified"
QT_MOC_LITERAL(24, 309, 11), // "markAsClean"
QT_MOC_LITERAL(25, 321, 21), // "updateCommonUsernames"
QT_MOC_LITERAL(26, 343, 4), // "topN"
QT_MOC_LITERAL(27, 348, 13), // "updateTagList"
QT_MOC_LITERAL(28, 362, 17) // "markNonDataChange"

    },
    "Database\0filePathChanged\0\0oldPath\0"
    "newPath\0groupDataChanged\0Group*\0group\0"
    "groupAboutToAdd\0index\0groupAdded\0"
    "groupAboutToRemove\0groupRemoved\0"
    "groupAboutToMove\0toGroup\0groupMoved\0"
    "databaseOpened\0databaseSaved\0"
    "databaseDiscarded\0databaseFileChanged\0"
    "triggeredBySave\0databaseNonDataChanged\0"
    "tagListUpdated\0markAsModified\0markAsClean\0"
    "updateCommonUsernames\0topN\0updateTagList\0"
    "markNonDataChange"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Database[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,  114,    2, 0x06 /* Public */,
       5,    1,  119,    2, 0x06 /* Public */,
       8,    2,  122,    2, 0x06 /* Public */,
      10,    0,  127,    2, 0x06 /* Public */,
      11,    1,  128,    2, 0x06 /* Public */,
      12,    0,  131,    2, 0x06 /* Public */,
      13,    3,  132,    2, 0x06 /* Public */,
      15,    0,  139,    2, 0x06 /* Public */,
      16,    0,  140,    2, 0x06 /* Public */,
      17,    0,  141,    2, 0x06 /* Public */,
      18,    0,  142,    2, 0x06 /* Public */,
      19,    1,  143,    2, 0x06 /* Public */,
      21,    0,  146,    2, 0x06 /* Public */,
      22,    0,  147,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      23,    0,  148,    2, 0x0a /* Public */,
      24,    0,  149,    2, 0x0a /* Public */,
      25,    1,  150,    2, 0x0a /* Public */,
      25,    0,  153,    2, 0x2a /* Public | MethodCloned */,
      27,    0,  154,    2, 0x0a /* Public */,
      28,    0,  155,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 6, QMetaType::Int,    7,    9,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6, QMetaType::Int,    7,   14,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   20,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   26,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void Database::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Database *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->filePathChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->groupDataChanged((*reinterpret_cast< Group*(*)>(_a[1]))); break;
        case 2: _t->groupAboutToAdd((*reinterpret_cast< Group*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->groupAdded(); break;
        case 4: _t->groupAboutToRemove((*reinterpret_cast< Group*(*)>(_a[1]))); break;
        case 5: _t->groupRemoved(); break;
        case 6: _t->groupAboutToMove((*reinterpret_cast< Group*(*)>(_a[1])),(*reinterpret_cast< Group*(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 7: _t->groupMoved(); break;
        case 8: _t->databaseOpened(); break;
        case 9: _t->databaseSaved(); break;
        case 10: _t->databaseDiscarded(); break;
        case 11: _t->databaseFileChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->databaseNonDataChanged(); break;
        case 13: _t->tagListUpdated(); break;
        case 14: _t->markAsModified(); break;
        case 15: _t->markAsClean(); break;
        case 16: _t->updateCommonUsernames((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: _t->updateCommonUsernames(); break;
        case 18: _t->updateTagList(); break;
        case 19: _t->markNonDataChange(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Database::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::filePathChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Database::*)(Group * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupDataChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Database::*)(Group * , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupAboutToAdd)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupAdded)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Database::*)(Group * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupAboutToRemove)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupRemoved)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Database::*)(Group * , Group * , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupAboutToMove)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::groupMoved)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::databaseOpened)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::databaseSaved)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::databaseDiscarded)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (Database::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::databaseFileChanged)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::databaseNonDataChanged)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (Database::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Database::tagListUpdated)) {
                *result = 13;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Database::staticMetaObject = { {
    QMetaObject::SuperData::link<ModifiableObject::staticMetaObject>(),
    qt_meta_stringdata_Database.data,
    qt_meta_data_Database,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Database::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Database::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Database.stringdata0))
        return static_cast<void*>(this);
    return ModifiableObject::qt_metacast(_clname);
}

int Database::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ModifiableObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void Database::filePathChanged(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Database::groupDataChanged(Group * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Database::groupAboutToAdd(Group * _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Database::groupAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void Database::groupAboutToRemove(Group * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Database::groupRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void Database::groupAboutToMove(Group * _t1, Group * _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Database::groupMoved()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void Database::databaseOpened()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void Database::databaseSaved()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void Database::databaseDiscarded()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void Database::databaseFileChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void Database::databaseNonDataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void Database::tagListUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
