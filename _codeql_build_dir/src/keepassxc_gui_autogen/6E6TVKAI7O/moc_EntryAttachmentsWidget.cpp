/****************************************************************************
** Meta object code from reading C++ file 'EntryAttachmentsWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/EntryAttachmentsWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EntryAttachmentsWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_EntryAttachmentsWidget_t {
    QByteArrayData data[30];
    char stringdata0[489];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EntryAttachmentsWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EntryAttachmentsWidget_t qt_meta_stringdata_EntryAttachmentsWidget = {
    {
QT_MOC_LITERAL(0, 0, 22), // "EntryAttachmentsWidget"
QT_MOC_LITERAL(1, 23, 13), // "errorOccurred"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 5), // "error"
QT_MOC_LITERAL(4, 44, 15), // "readOnlyChanged"
QT_MOC_LITERAL(5, 60, 8), // "readOnly"
QT_MOC_LITERAL(6, 69, 21), // "buttonsVisibleChanged"
QT_MOC_LITERAL(7, 91, 16), // "isButtonsVisible"
QT_MOC_LITERAL(8, 108, 13), // "widgetUpdated"
QT_MOC_LITERAL(9, 122, 15), // "linkAttachments"
QT_MOC_LITERAL(10, 138, 17), // "EntryAttachments*"
QT_MOC_LITERAL(11, 156, 11), // "attachments"
QT_MOC_LITERAL(12, 168, 17), // "unlinkAttachments"
QT_MOC_LITERAL(13, 186, 11), // "setReadOnly"
QT_MOC_LITERAL(14, 198, 17), // "setButtonsVisible"
QT_MOC_LITERAL(15, 216, 17), // "insertAttachments"
QT_MOC_LITERAL(16, 234, 14), // "newAttachments"
QT_MOC_LITERAL(17, 249, 22), // "editSelectedAttachment"
QT_MOC_LITERAL(18, 272, 25), // "previewSelectedAttachment"
QT_MOC_LITERAL(19, 298, 25), // "removeSelectedAttachments"
QT_MOC_LITERAL(20, 324, 23), // "saveSelectedAttachments"
QT_MOC_LITERAL(21, 348, 14), // "openAttachment"
QT_MOC_LITERAL(22, 363, 11), // "QModelIndex"
QT_MOC_LITERAL(23, 375, 5), // "index"
QT_MOC_LITERAL(24, 381, 23), // "openSelectedAttachments"
QT_MOC_LITERAL(25, 405, 20), // "updateButtonsVisible"
QT_MOC_LITERAL(26, 426, 20), // "updateButtonsEnabled"
QT_MOC_LITERAL(27, 447, 28), // "attachmentModifiedExternally"
QT_MOC_LITERAL(28, 476, 3), // "key"
QT_MOC_LITERAL(29, 480, 8) // "filePath"

    },
    "EntryAttachmentsWidget\0errorOccurred\0"
    "\0error\0readOnlyChanged\0readOnly\0"
    "buttonsVisibleChanged\0isButtonsVisible\0"
    "widgetUpdated\0linkAttachments\0"
    "EntryAttachments*\0attachments\0"
    "unlinkAttachments\0setReadOnly\0"
    "setButtonsVisible\0insertAttachments\0"
    "newAttachments\0editSelectedAttachment\0"
    "previewSelectedAttachment\0"
    "removeSelectedAttachments\0"
    "saveSelectedAttachments\0openAttachment\0"
    "QModelIndex\0index\0openSelectedAttachments\0"
    "updateButtonsVisible\0updateButtonsEnabled\0"
    "attachmentModifiedExternally\0key\0"
    "filePath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EntryAttachmentsWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       2,  146, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  109,    2, 0x06 /* Public */,
       4,    1,  112,    2, 0x06 /* Public */,
       6,    1,  115,    2, 0x06 /* Public */,
       8,    0,  118,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    1,  119,    2, 0x0a /* Public */,
      12,    0,  122,    2, 0x0a /* Public */,
      13,    1,  123,    2, 0x0a /* Public */,
      14,    1,  126,    2, 0x0a /* Public */,
      15,    0,  129,    2, 0x08 /* Private */,
      16,    0,  130,    2, 0x08 /* Private */,
      17,    0,  131,    2, 0x08 /* Private */,
      18,    0,  132,    2, 0x08 /* Private */,
      19,    0,  133,    2, 0x08 /* Private */,
      20,    0,  134,    2, 0x08 /* Private */,
      21,    1,  135,    2, 0x08 /* Private */,
      24,    0,  138,    2, 0x08 /* Private */,
      25,    0,  139,    2, 0x08 /* Private */,
      26,    0,  140,    2, 0x08 /* Private */,
      27,    2,  141,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   28,   29,

 // properties: name, type, flags
       5, QMetaType::Bool, 0x00495103,
       7, QMetaType::Bool, 0x00495003,

 // properties: notify_signal_id
       1,
       2,

       0        // eod
};

void EntryAttachmentsWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntryAttachmentsWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->readOnlyChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->buttonsVisibleChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->widgetUpdated(); break;
        case 4: _t->linkAttachments((*reinterpret_cast< EntryAttachments*(*)>(_a[1]))); break;
        case 5: _t->unlinkAttachments(); break;
        case 6: _t->setReadOnly((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->setButtonsVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->insertAttachments(); break;
        case 9: _t->newAttachments(); break;
        case 10: _t->editSelectedAttachment(); break;
        case 11: _t->previewSelectedAttachment(); break;
        case 12: _t->removeSelectedAttachments(); break;
        case 13: _t->saveSelectedAttachments(); break;
        case 14: _t->openAttachment((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 15: _t->openSelectedAttachments(); break;
        case 16: _t->updateButtonsVisible(); break;
        case 17: _t->updateButtonsEnabled(); break;
        case 18: _t->attachmentModifiedExternally((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EntryAttachmentsWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachmentsWidget::errorOccurred)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EntryAttachmentsWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachmentsWidget::readOnlyChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EntryAttachmentsWidget::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachmentsWidget::buttonsVisibleChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (EntryAttachmentsWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&EntryAttachmentsWidget::widgetUpdated)) {
                *result = 3;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<EntryAttachmentsWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isReadOnly(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->isButtonsVisible(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<EntryAttachmentsWidget *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setReadOnly(*reinterpret_cast< bool*>(_v)); break;
        case 1: _t->setButtonsVisible(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject EntryAttachmentsWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_EntryAttachmentsWidget.data,
    qt_meta_data_EntryAttachmentsWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *EntryAttachmentsWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EntryAttachmentsWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_EntryAttachmentsWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int EntryAttachmentsWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 19;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void EntryAttachmentsWidget::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EntryAttachmentsWidget::readOnlyChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EntryAttachmentsWidget::buttonsVisibleChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void EntryAttachmentsWidget::widgetUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
