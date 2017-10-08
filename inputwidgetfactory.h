#ifndef INPUTWIDGETFACTORY_H
#define INPUTWIDGETFACTORY_H

#include <QMetaProperty>
#include <QWidget>
#include <functional>

class InputWidgetFactory
{
public:
	InputWidgetFactory();
	virtual ~InputWidgetFactory();

	virtual QWidget *createWidget(const QByteArray &type, QWidget *parent, const QVariantMap &editProperties);
	virtual QMetaProperty userProperty(QWidget *widget);

	virtual bool addSimpleWidget(const QByteArray &type, const std::function<QWidget*(QWidget*)> &creator);
	template <typename TWidget>
	bool addSimpleWidget(const QByteArray &type);

protected:
	virtual QWidget *createListWidget(QWidget *parent, const QVariantMap &editProperties);

	QHash<QByteArray, std::function<QWidget*(QWidget*)>> _simpleWidgets;
};

template<typename TWidget>
bool InputWidgetFactory::addSimpleWidget(const QByteArray &type)
{
	return addSimpleWidget(type, [](QWidget *parent){
		return new TWidget(parent);
	});
}

#endif // INPUTWIDGETFACTORY_H
