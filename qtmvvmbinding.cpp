#include "qtmvvmbinding.h"
#include <QDebug>

QtMvvmBinding::QtMvvmBinding(QObject *control, const QMetaProperty &controlProperty, QObject *widget, const QMetaProperty &widgetProperty) :
	QObject(widget),
	_control(control),
	_widget(widget),
	_controlProperty(controlProperty),
	_widgetProperty(widgetProperty)
{
	connect(_control, &QObject::destroyed,
			this, &QtMvvmBinding::deleteLater);
}

QtMvvmBinding *QtMvvmBinding::bind(QObject *control, const char *controlProperty, QObject *widget, const char *widgetProperty, BindingDirection type)
{
	if(!control) {
		qWarning() << "Control must not be (null)";
		return nullptr;
	}
	auto mC = control->metaObject();
	auto piC = mC->indexOfProperty(controlProperty);
	if(piC == -1) {
		qWarning() << "Control of type" << mC->className() << "has no property named" << controlProperty;
		return nullptr;
	}

	if(!widget) {
		qWarning() << "Widget must not be (null)";
		return nullptr;
	}
	auto mW = widget->metaObject();
	auto piW = mW->indexOfProperty(widgetProperty);
	if(piW == -1) {
		qWarning() << "Control of type" << mW->className() << "has no property named" << widgetProperty;
		return nullptr;
	}

	return bind(control, mC->property(piC), widget, mW->property(piW), type);
}

QtMvvmBinding *QtMvvmBinding::bind(QObject *control, const QMetaProperty &controlProperty, QObject *widget, const QMetaProperty &widgetProperty, BindingDirection type)
{
	//check not null
	if(!control) {
		qWarning() << "Control must not be (null)";
		return nullptr;
	}
	if(!widget) {
		qWarning() << "Widget must not be (null)";
		return nullptr;
	}

	auto binder = new QtMvvmBinding(control, controlProperty, widget, widgetProperty);

	if(type.testFlag(SingleInit)) {
		if(!controlProperty.isReadable()) {
			qWarning() << "Control property" << controlProperty.name()
					   << "of" << controlProperty.enclosingMetaObject()->className()
					   << "is not readable";
		}
		if(!widgetProperty.isWritable()) {
			qWarning() << "Widget property" << widgetProperty.name()
					   << "of" << widgetProperty.enclosingMetaObject()->className()
					   << "is not writable";
		}
		binder->init();
	}

	if(type.testFlag(OneWayFromControl)) {
		if(!controlProperty.hasNotifySignal()) {
			qWarning() << "Control property" << controlProperty.name()
					   << "of" << controlProperty.enclosingMetaObject()->className()
					   << "has no notify singal";
		}
		binder->bindFrom();
	}

	if(type.testFlag(OneWayToControl)) {
		if(!widgetProperty.isReadable()) {
			qWarning() << "Widget property" << widgetProperty.name()
					   << "of" << widgetProperty.enclosingMetaObject()->className()
					   << "is not readable";
		}
		if(!controlProperty.isWritable()) {
			qWarning() << "Control property" << controlProperty.name()
					   << "of" << controlProperty.enclosingMetaObject()->className()
					   << "is not writable";
		}
		if(!widgetProperty.hasNotifySignal()) {
			qWarning() << "Widget property" << widgetProperty.name()
					   << "of" << widgetProperty.enclosingMetaObject()->className()
					   << "has no notify singal";
		}
		binder->bindTo();
	}

	return binder;
}

void QtMvvmBinding::controlTrigger()
{
	_widgetProperty.write(_widget, _controlProperty.read(_control));
}

void QtMvvmBinding::widgetTrigger()
{
	_controlProperty.write(_control, _widgetProperty.read(_widget));
}

void QtMvvmBinding::init()
{
	_widgetProperty.write(_widget, _controlProperty.read(_control));
}

void QtMvvmBinding::bindFrom()
{
	auto signal = _controlProperty.notifySignal();
	auto trigger = metaObject()->method(metaObject()->indexOfSlot("controlTrigger()"));
	connect(_control, signal, this, trigger);
}

void QtMvvmBinding::bindTo()
{
	auto signal = _widgetProperty.notifySignal();
	auto trigger = metaObject()->method(metaObject()->indexOfSlot("widgetTrigger()"));
	connect(_widget, signal, this, trigger);
}
