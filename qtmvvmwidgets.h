#ifndef QTMVVMWIDGETS_H
#define QTMVVMWIDGETS_H

#include <QObject>
#include <QMetaProperty>
#include <QtGlobal>

class QtMvvmBinding : public QObject
{
	Q_OBJECT

public:
	enum BindingDirectionFlag {
		SingleInit = 0x01,
		OneWayFromControl = (0x02 | SingleInit),
		OneWayToControl = 0x04,
		TwoWay = (OneWayFromControl | OneWayToControl)
	};
	Q_DECLARE_FLAGS(BindingDirection, BindingDirectionFlag)
	Q_FLAG(BindingDirection)

	static QtMvvmBinding *bind(QObject *control, const char *controlProperty, QObject *widget, const char *widgetProperty, BindingDirection type = TwoWay);
	static QtMvvmBinding *bind(QObject *control, const QMetaProperty &controlProperty, QObject *widget, const QMetaProperty &widgetProperty, BindingDirection type = TwoWay);

private slots:
	void controlTrigger();
	void widgetTrigger();

private:
	QtMvvmBinding(QObject *control, const QMetaProperty &controlProperty, QObject *widget, const QMetaProperty &widgetProperty);

	void init();
	void bindFrom();
	void bindTo();

	QObject *_control;
	QObject *_widget;
	QMetaProperty _controlProperty;
	QMetaProperty _widgetProperty;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QtMvvmBinding::BindingDirection)

#endif // QTMVVMWIDGETS_H
