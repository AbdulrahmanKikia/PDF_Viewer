/****************************************************************************
** Meta object code from reading C++ file 'PdfViewerWidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/ui/pdf/PdfViewerWidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PdfViewerWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN15PdfViewerWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto PdfViewerWidget::qt_create_metaobjectdata<qt_meta_tag_ZN15PdfViewerWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "PdfViewerWidget",
        "titleChanged",
        "",
        "title",
        "onPageRendered",
        "RenderKey",
        "key",
        "QImage",
        "image",
        "onRenderError",
        "message",
        "onRenderPageRequested",
        "pageIndex",
        "renderVer",
        "QSize",
        "targetSize",
        "zoomFactor",
        "onZoomChanged",
        "pct",
        "onCurrentPageChanged",
        "onPrevPage",
        "onNextPage",
        "onZoomIn",
        "onZoomOut",
        "onZoomComboChanged",
        "index",
        "onSpinBoxCommitted",
        "onToggleSidebar",
        "onContinuousModeClicked",
        "onSinglePageModeClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'titleChanged'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onPageRendered'
        QtMocHelpers::SlotData<void(const RenderKey &, const QImage &)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 5, 6 }, { 0x80000000 | 7, 8 },
        }}),
        // Slot 'onRenderError'
        QtMocHelpers::SlotData<void(const RenderKey &, const QString &)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 5, 6 }, { QMetaType::QString, 10 },
        }}),
        // Slot 'onRenderPageRequested'
        QtMocHelpers::SlotData<void(int, int, QSize, double)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 12 }, { QMetaType::Int, 13 }, { 0x80000000 | 14, 15 }, { QMetaType::Double, 16 },
        }}),
        // Slot 'onZoomChanged'
        QtMocHelpers::SlotData<void(double)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Double, 18 },
        }}),
        // Slot 'onCurrentPageChanged'
        QtMocHelpers::SlotData<void(int)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 12 },
        }}),
        // Slot 'onPrevPage'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onNextPage'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onZoomIn'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onZoomOut'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onZoomComboChanged'
        QtMocHelpers::SlotData<void(int)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 25 },
        }}),
        // Slot 'onSpinBoxCommitted'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToggleSidebar'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onContinuousModeClicked'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSinglePageModeClicked'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PdfViewerWidget, qt_meta_tag_ZN15PdfViewerWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject PdfViewerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15PdfViewerWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15PdfViewerWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15PdfViewerWidgetE_t>.metaTypes,
    nullptr
} };

void PdfViewerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PdfViewerWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->titleChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onPageRendered((*reinterpret_cast<std::add_pointer_t<RenderKey>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QImage>>(_a[2]))); break;
        case 2: _t->onRenderError((*reinterpret_cast<std::add_pointer_t<RenderKey>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->onRenderPageRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QSize>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[4]))); break;
        case 4: _t->onZoomChanged((*reinterpret_cast<std::add_pointer_t<double>>(_a[1]))); break;
        case 5: _t->onCurrentPageChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->onPrevPage(); break;
        case 7: _t->onNextPage(); break;
        case 8: _t->onZoomIn(); break;
        case 9: _t->onZoomOut(); break;
        case 10: _t->onZoomComboChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->onSpinBoxCommitted(); break;
        case 12: _t->onToggleSidebar(); break;
        case 13: _t->onContinuousModeClicked(); break;
        case 14: _t->onSinglePageModeClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PdfViewerWidget::*)(const QString & )>(_a, &PdfViewerWidget::titleChanged, 0))
            return;
    }
}

const QMetaObject *PdfViewerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PdfViewerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15PdfViewerWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int PdfViewerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void PdfViewerWidget::titleChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
