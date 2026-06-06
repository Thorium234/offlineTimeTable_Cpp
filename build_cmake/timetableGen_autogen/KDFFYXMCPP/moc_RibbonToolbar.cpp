/****************************************************************************
** Meta object code from reading C++ file 'RibbonToolbar.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../gui/ribbon/RibbonToolbar.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'RibbonToolbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RibbonToolbar_t {
    QByteArrayData data[21];
    char stringdata0[302];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RibbonToolbar_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RibbonToolbar_t qt_meta_stringdata_RibbonToolbar = {
    {
QT_MOC_LITERAL(0, 0, 13), // "RibbonToolbar"
QT_MOC_LITERAL(1, 14, 15), // "generateClicked"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 16), // "exportCsvClicked"
QT_MOC_LITERAL(4, 48, 17), // "exportHtmlClicked"
QT_MOC_LITERAL(5, 66, 16), // "exportPdfClicked"
QT_MOC_LITERAL(6, 83, 19), // "printPreviewClicked"
QT_MOC_LITERAL(7, 103, 16), // "benchmarkClicked"
QT_MOC_LITERAL(8, 120, 15), // "viewModeChanged"
QT_MOC_LITERAL(9, 136, 8), // "ViewMode"
QT_MOC_LITERAL(10, 145, 4), // "mode"
QT_MOC_LITERAL(11, 150, 11), // "homeClicked"
QT_MOC_LITERAL(12, 162, 16), // "timetableClicked"
QT_MOC_LITERAL(13, 179, 11), // "undoClicked"
QT_MOC_LITERAL(14, 191, 11), // "redoClicked"
QT_MOC_LITERAL(15, 203, 11), // "lockClicked"
QT_MOC_LITERAL(16, 215, 22), // "constraintsModeToggled"
QT_MOC_LITERAL(17, 238, 7), // "enabled"
QT_MOC_LITERAL(18, 246, 20), // "substitutionsClicked"
QT_MOC_LITERAL(19, 267, 16), // "divisionsClicked"
QT_MOC_LITERAL(20, 284, 17) // "violationsClicked"

    },
    "RibbonToolbar\0generateClicked\0\0"
    "exportCsvClicked\0exportHtmlClicked\0"
    "exportPdfClicked\0printPreviewClicked\0"
    "benchmarkClicked\0viewModeChanged\0"
    "ViewMode\0mode\0homeClicked\0timetableClicked\0"
    "undoClicked\0redoClicked\0lockClicked\0"
    "constraintsModeToggled\0enabled\0"
    "substitutionsClicked\0divisionsClicked\0"
    "violationsClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RibbonToolbar[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      16,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x06 /* Public */,
       3,    0,   95,    2, 0x06 /* Public */,
       4,    0,   96,    2, 0x06 /* Public */,
       5,    0,   97,    2, 0x06 /* Public */,
       6,    0,   98,    2, 0x06 /* Public */,
       7,    0,   99,    2, 0x06 /* Public */,
       8,    1,  100,    2, 0x06 /* Public */,
      11,    0,  103,    2, 0x06 /* Public */,
      12,    0,  104,    2, 0x06 /* Public */,
      13,    0,  105,    2, 0x06 /* Public */,
      14,    0,  106,    2, 0x06 /* Public */,
      15,    0,  107,    2, 0x06 /* Public */,
      16,    1,  108,    2, 0x06 /* Public */,
      18,    0,  111,    2, 0x06 /* Public */,
      19,    0,  112,    2, 0x06 /* Public */,
      20,    0,  113,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void RibbonToolbar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RibbonToolbar *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->generateClicked(); break;
        case 1: _t->exportCsvClicked(); break;
        case 2: _t->exportHtmlClicked(); break;
        case 3: _t->exportPdfClicked(); break;
        case 4: _t->printPreviewClicked(); break;
        case 5: _t->benchmarkClicked(); break;
        case 6: _t->viewModeChanged((*reinterpret_cast< ViewMode(*)>(_a[1]))); break;
        case 7: _t->homeClicked(); break;
        case 8: _t->timetableClicked(); break;
        case 9: _t->undoClicked(); break;
        case 10: _t->redoClicked(); break;
        case 11: _t->lockClicked(); break;
        case 12: _t->constraintsModeToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->substitutionsClicked(); break;
        case 14: _t->divisionsClicked(); break;
        case 15: _t->violationsClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::generateClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::exportCsvClicked)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::exportHtmlClicked)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::exportPdfClicked)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::printPreviewClicked)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::benchmarkClicked)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)(ViewMode );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::viewModeChanged)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::homeClicked)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::timetableClicked)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::undoClicked)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::redoClicked)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::lockClicked)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::constraintsModeToggled)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::substitutionsClicked)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::divisionsClicked)) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (RibbonToolbar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RibbonToolbar::violationsClicked)) {
                *result = 15;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RibbonToolbar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_RibbonToolbar.data,
    qt_meta_data_RibbonToolbar,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RibbonToolbar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RibbonToolbar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RibbonToolbar.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int RibbonToolbar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void RibbonToolbar::generateClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RibbonToolbar::exportCsvClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void RibbonToolbar::exportHtmlClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void RibbonToolbar::exportPdfClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void RibbonToolbar::printPreviewClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void RibbonToolbar::benchmarkClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void RibbonToolbar::viewModeChanged(ViewMode _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void RibbonToolbar::homeClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void RibbonToolbar::timetableClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void RibbonToolbar::undoClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void RibbonToolbar::redoClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void RibbonToolbar::lockClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void RibbonToolbar::constraintsModeToggled(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void RibbonToolbar::substitutionsClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void RibbonToolbar::divisionsClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void RibbonToolbar::violationsClicked()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
