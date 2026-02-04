QT       += core gui widgets network printsupport sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Disabilita deprecation warnings per API Qt obsolete
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    shellymanager.cpp \
    deviceinfotab.cpp \
    settingstab.cpp \
    shellycsvimporter.cpp \
    databasemanager.cpp \
    historyviewertab.cpp \
    alarmmanager.cpp \
    logger.cpp \
    logviewerdialog.cpp \
    statisticstab.cpp \
    qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    shellymanager.h \
    datapoint.h \
    deviceinfo.h \
    deviceinfotab.h \
    settingstab.h \
    shellycsvimporter.h \
    databasemanager.h \
    historyviewertab.h \
    alarmmanager.h \
    logger.h \
    logviewerdialog.h \
    aggregateddata.h \
    statisticstab.h \
    qcustomplot.h

# Qt Resource file
RESOURCES += resources.qrc

# Windows icon
win32:RC_ICONS = resources/icons/icon.ico

# File di traduzione
TRANSLATIONS += \
    translations/shelly_logger_it.ts \
    translations/shelly_logger_en.ts \
    translations/shelly_logger_fr.ts \
    translations/shelly_logger_de.ts \
    translations/shelly_logger_es.ts

# Impostazioni di default per target
TARGET = shelly_logger
TEMPLATE = app

# Configurazione output directories
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
