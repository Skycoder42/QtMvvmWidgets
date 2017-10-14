INCLUDEPATH += $$PWD

HEADERS += \
	$$PWD/widgetpresenter.h \
	$$PWD/ipresentingwidget.h \
	$$PWD/inputwidgetfactory.h \
	$$PWD/listcombobox.h \
    $$PWD/qtmvvmbinding.h

SOURCES += \
	$$PWD/widgetpresenter.cpp \
	$$PWD/inputwidgetfactory.cpp \
	$$PWD/listcombobox.cpp \
    $$PWD/qtmvvmbinding.cpp

TRANSLATIONS += $$PWD/qtmvvm_widgets_de.ts \
	$$PWD/qtmvvm_widgets_template.ts
