/****************************************************************************
** Meta object code from reading C++ file 'TextAttachmentsPreviewWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/gui/entry/attachments/TextAttachmentsPreviewWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TextAttachmentsPreviewWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TextAttachmentsPreviewWidget_t {
    QByteArrayData data[10];
    char stringdata0[110];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TextAttachmentsPreviewWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TextAttachmentsPreviewWidget_t qt_meta_stringdata_TextAttachmentsPreviewWidget = {
    {
QT_MOC_LITERAL(0, 0, 28), // "TextAttachmentsPreviewWidget"
QT_MOC_LITERAL(1, 29, 11), // "matchScroll"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 7), // "percent"
QT_MOC_LITERAL(4, 50, 13), // "onTypeChanged"
QT_MOC_LITERAL(5, 64, 5), // "index"
QT_MOC_LITERAL(6, 70, 15), // "PreviewTextType"
QT_MOC_LITERAL(7, 86, 4), // "Html"
QT_MOC_LITERAL(8, 91, 9), // "PlainText"
QT_MOC_LITERAL(9, 101, 8) // "Markdown"

    },
    "TextAttachmentsPreviewWidget\0matchScroll\0"
    "\0percent\0onTypeChanged\0index\0"
    "PreviewTextType\0Html\0PlainText\0Markdown"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TextAttachmentsPreviewWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       1,   30, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x0a /* Public */,
       4,    1,   27,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Int,    5,

 // enums: name, alias, flags, count, data
       6,    6, 0x0,    3,   35,

 // enum data: key, value
       7, uint(TextAttachmentsPreviewWidget::Html),
       8, uint(TextAttachmentsPreviewWidget::PlainText),
       9, uint(TextAttachmentsPreviewWidget::Markdown),

       0        // eod
};

void TextAttachmentsPreviewWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TextAttachmentsPreviewWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->matchScroll((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 1: _t->onTypeChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TextAttachmentsPreviewWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TextAttachmentsPreviewWidget.data,
    qt_meta_data_TextAttachmentsPreviewWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TextAttachmentsPreviewWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TextAttachmentsPreviewWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TextAttachmentsPreviewWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TextAttachmentsPreviewWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
