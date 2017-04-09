#include "ipresentingwidget.h"
#include "widgetpresenter.h"

#include <QDebug>
#include <QMainWindow>
#include <QDialog>
#include <QDockWidget>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QApplication>
#include <float.h>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <dialogmaster.h>

#define INPUT_WIDGET_OBJECT_NAME "__qtmvvm_InputDialog_InputWidget"

WidgetPresenter::WidgetPresenter() :
	_inputFactory(new InputWidgetFactory()),
	_implicitMappings(),
	_explicitMappings(),
	_activeControls()
{}

void WidgetPresenter::registerWidget(const QMetaObject &widgetType)
{
	auto presenter = static_cast<WidgetPresenter*>(CoreApp::instance()->presenter());
	if(!presenter) {
		presenter = new WidgetPresenter();
		CoreApp::setMainPresenter(presenter);
	}
	presenter->_implicitMappings.append(widgetType);
}

void WidgetPresenter::registerWidgetExplicitly(const char *controlName, const QMetaObject &widgetType)
{
	auto presenter = static_cast<WidgetPresenter*>(CoreApp::instance()->presenter());
	if(!presenter) {
		presenter = new WidgetPresenter();
		CoreApp::setMainPresenter(presenter);
	}
	presenter->_explicitMappings.insert(controlName, widgetType);
}

void WidgetPresenter::registerInputWidgetFactory(InputWidgetFactory *factory)
{
	auto presenter = static_cast<WidgetPresenter*>(CoreApp::instance()->presenter());
	if(!presenter) {
		presenter = new WidgetPresenter();
		CoreApp::setMainPresenter(presenter);
	}
	presenter->_inputFactory.reset(factory);
}

InputWidgetFactory *WidgetPresenter::inputWidgetFactory()
{
	auto presenter = static_cast<WidgetPresenter*>(CoreApp::instance()->presenter());
	if(presenter)
		return presenter->_inputFactory.data();
	else
		return nullptr;
}

void WidgetPresenter::present(Control *control)
{
	if(_activeControls.contains(control))
		return;

	auto ok = false;
	auto widgetMetaObject = findWidgetMetaObject(control->metaObject(), ok);
	if(ok) {
		auto parentControl = control->parentControl();
		auto parentWidget = parentControl ? _activeControls.value(parentControl, nullptr) : nullptr;

		auto widget = qobject_cast<QWidget*>(widgetMetaObject.newInstance(Q_ARG(Control *, control),
																		  Q_ARG(QWidget*, parentWidget)));
		if(!widget) {
			qCritical() << "Failed to create widget of type"
						<< widgetMetaObject.className()
						<< "(did you mark the constructor as Q_INVOKABLE? Required signature: \"Contructor(Control *, QWidget*);\")";
			return;
		}

		auto presented = false;
		auto tPresenter = dynamic_cast<IPresentingWidget*>(parentWidget);
		if(tPresenter)
			presented = tPresenter->tryPresent(widget);
		if(!presented)
			presented = tryPresent(widget, parentWidget);

		if(presented) {
			_activeControls.insert(control, widget);
			widget->setAttribute(Qt::WA_DeleteOnClose);
			QObject::connect(widget, &QWidget::destroyed, [=](){
				_activeControls.remove(control);
				control->onClose();
			});
			control->onShow();
		} else {
			qCritical() << "Failed to present created widget";
			widget->deleteLater();
		}
	} else {
		qCritical() << "Unable to find widget for control of type"
					<< control->metaObject()->className();
	}
}

void WidgetPresenter::withdraw(Control *control)
{
	auto widget = _activeControls.value(control);
	if(widget)
		widget->close();
	else {
		qWarning() << "control of type"
				   << control->metaObject()->className()
				   << "is currently not shown, and thus will not be withdrawn";
	}
}

void WidgetPresenter::showMessage(MessageResult *result, const CoreApp::MessageConfig &config)
{
	QDialog *dialog = nullptr;
	if(config.type == CoreApp::Input)
		dialog = createInputDialog(config);
	else {
		auto msgBox = new QMessageBox();
		if(config.title.isEmpty())
			msgBox->setText(config.text);
		else {
			msgBox->setText(QStringLiteral("<b>%1</b>").arg(config.title));
			msgBox->setInformativeText(config.text);
		}

		QPushButton *aBtn = nullptr;
		QPushButton *rBtn = nullptr;
		QPushButton *nBtn = nullptr;
		if(!config.positiveAction.isNull())
			aBtn = msgBox->addButton(config.positiveAction, QMessageBox::AcceptRole);
		if(!config.negativeAction.isNull())
			rBtn = msgBox->addButton(config.negativeAction, QMessageBox::RejectRole);
		if(!config.neutralAction.isNull())
			nBtn = msgBox->addButton(config.neutralAction, QMessageBox::DestructiveRole);

		//default button
		if(aBtn)
			msgBox->setDefaultButton(aBtn);
		else if(rBtn)
			msgBox->setDefaultButton(rBtn);
		else if(nBtn)
			msgBox->setDefaultButton(nBtn);

		//escape button
		if(rBtn)
			msgBox->setEscapeButton(rBtn);
		else if(aBtn)
			msgBox->setEscapeButton(aBtn);
		else if(nBtn)
			msgBox->setEscapeButton(nBtn);

		switch (config.type) {
		case CoreApp::Information:
			msgBox->setIcon(QMessageBox::Information);
			msgBox->setWindowTitle("Information");
			break;
		case CoreApp::Question:
			msgBox->setIcon(QMessageBox::Question);
			msgBox->setWindowTitle(tr("Question"));
			break;
		case CoreApp::Warning:
			msgBox->setIcon(QMessageBox::Warning);
			msgBox->setWindowTitle(tr("Warning"));
			break;
		case CoreApp::Critical:
			msgBox->setIcon(QMessageBox::Critical);
			msgBox->setWindowTitle(tr("Error"));
			break;
		default:
			Q_UNREACHABLE();
			break;
		}

		dialog = msgBox;
	}

	if(dialog) {
		dialog->setParent(QApplication::activeWindow());
		dialog->setAttribute(Qt::WA_DeleteOnClose);
		DialogMaster::masterDialog(dialog, true);

		QObject::connect(dialog, &QDialog::finished, dialog, [=](){
			auto res = getResult(dialog);
			QVariant value;
			if(config.type == CoreApp::Input && res == MessageResult::PositiveResult)
				value = extractInputResult(dialog);
			result->complete(res, value);
		});
		result->setCloseTarget(dialog, QDialog::staticMetaObject.method(QDialog::staticMetaObject.indexOfSlot("close()")));

		dialog->open();
	} else {
		qCritical() << "Failed to create message dialog for message type"
					<< config.type
					<< "and input type"
					<< config.inputType;
		result->complete(MessageResult::NegativeResult, {});
	}
}

QMetaObject WidgetPresenter::findWidgetMetaObject(const QMetaObject *controlMetaObject, bool &ok)
{
	ok = true;
	auto currentMeta = controlMetaObject;
	while(currentMeta && currentMeta->inherits(&Control::staticMetaObject)) {
		QByteArray cName = currentMeta->className();
		if(_explicitMappings.contains(cName))
			return _explicitMappings.value(cName);
		else {
			auto lIndex = cName.lastIndexOf("Control");
			if(lIndex > 0)
				cName.truncate(lIndex);
			foreach(auto metaObject, _implicitMappings) {
				QByteArray wName = metaObject.className();
				if(wName.startsWith(cName))
					return metaObject;
			}
		}

		currentMeta = currentMeta->superClass();
	}

	ok = false;
	return QMetaObject();
}

bool WidgetPresenter::tryPresent(QWidget *widget, QWidget *parent)
{
	auto metaObject = widget->metaObject();
	if(metaObject->inherits(&QDialog::staticMetaObject)) {
		qobject_cast<QDialog*>(widget)->open();
		return true;
	} else if(parent && parent->metaObject()->inherits(&QMainWindow::staticMetaObject)) {
		auto mainWindow = qobject_cast<QMainWindow*>(parent);
		if(metaObject->inherits(&QDockWidget::staticMetaObject)) {
			auto dockWidget = qobject_cast<QDockWidget*>(widget);
			if(!mainWindow->restoreDockWidget(dockWidget))
				mainWindow->addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
			return true;
		} else if(metaObject->inherits(&QMdiSubWindow::staticMetaObject) &&
		   mainWindow->centralWidget() &&
		   mainWindow->centralWidget()->metaObject()->inherits(&QMdiArea::staticMetaObject)) {
			auto mdiArea = qobject_cast<QMdiArea*>(mainWindow->centralWidget());
			mdiArea->addSubWindow(widget);
			return true;
		}
	}

	extendedShow(widget);
	return true;
}

QDialog *WidgetPresenter::createInputDialog(const CoreApp::MessageConfig &config)
{
	auto input = _inputFactory->createWidget(config.inputType, nullptr, config.editProperties);
	if(!input)
		return nullptr;

	auto dialog = new QDialog();
	dialog->setModal(true);
	auto layout = new QVBoxLayout(dialog);
	dialog->setLayout(layout);
	dialog->setWindowTitle(config.title);
	if(!config.text.isNull()) {
		auto label = new QLabel(config.text, dialog);
		layout->addWidget(label);
	}

	input->setParent(dialog);
	input->setObjectName(INPUT_WIDGET_OBJECT_NAME);
	auto property = _inputFactory->userProperty(input);
	property.write(input, config.defaultValue);
	layout->addWidget(input);

	auto btnBox = new QDialogButtonBox(dialog);
	QObject::connect(btnBox, &QDialogButtonBox::accepted,
					 dialog, &QDialog::accept);
	QObject::connect(btnBox, &QDialogButtonBox::rejected,
					 dialog, &QDialog::reject);
	if(!config.positiveAction.isNull())
		btnBox->addButton(config.positiveAction, QDialogButtonBox::AcceptRole);
	else
		btnBox->addButton(QDialogButtonBox::Ok);
	if(!config.negativeAction.isNull())
		btnBox->addButton(config.negativeAction, QDialogButtonBox::RejectRole);
	else
		btnBox->addButton(QDialogButtonBox::Cancel);
	layout->addWidget(btnBox);

	dialog->adjustSize();
	return dialog;
}

QVariant WidgetPresenter::extractInputResult(QDialog *inputDialog)
{
	auto inputWidget = inputDialog->findChild<QWidget*>(INPUT_WIDGET_OBJECT_NAME);
	if(inputWidget) {
		auto property = _inputFactory->userProperty(inputWidget);
		return property.read(inputWidget);
	} else
		return {};
}

void WidgetPresenter::extendedShow(QWidget *widget) const
{
	widget->setWindowState(widget->windowState() & ~ Qt::WindowMinimized);
	widget->show();
	widget->raise();
	QApplication::alert(widget);
	widget->activateWindow();
}

MessageResult::ResultType WidgetPresenter::getResult(QDialog *dialog)
{
	auto msgBox = qobject_cast<QMessageBox*>(dialog);
	if(msgBox) {
		switch(msgBox->buttonRole(msgBox->clickedButton())) {
		case QMessageBox::AcceptRole:
			return MessageResult::PositiveResult;
		case QMessageBox::RejectRole:
			return MessageResult::NegativeResult;
		default:
			return MessageResult::NeutralResult;
		}
	} else {
		switch (dialog->result()) {
		case QDialog::Accepted:
			return MessageResult::PositiveResult;
		case QDialog::Rejected:
			return MessageResult::NegativeResult;
		default:
			return MessageResult::NeutralResult;
		}
	}
}
