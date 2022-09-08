TEMPLATE    = lib

TARGET      = qtnribbondsgn

!debug_and_release|build_pass {
    CONFIG(debug, debug|release) {
        TARGET = $$member(TARGET, 0)d
    }
}

QTITANDIR = $$(QTITANDIR)
include($$QTITANDIR/src/shared/platform.pri)

DESTDIR = $$PWD/../../bin
DESTDIR = $$member(DESTDIR, 0)$$QTITAN_LIB_PREFIX

VERSION = 3.2.1

QMAKE_TARGET_COMPANY = Developer Machines
QMAKE_TARGET_PRODUCT = QtitanRibbon for Qt
QMAKE_TARGET_DESCRIPTION = QtitanRibbon Plugin for QtDesigner
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2009-2014 Developer Machines

contains(CONFIG, demo) {
    QMAKE_TARGET_DESCRIPTION = QtitanRibbon Plugin for QtDesigner (Demo version)
}

include($$PWD/../ribbon/ribbon.pri)
include($$PWD/../styles/styles.pri)

HEADERS     += $$PWD/QtnRibbonDsgnPlugin.h \
               $$PWD/QtnRibbonDsgnContainer.h \
               $$PWD/QtnRibbonDsgnTaskMenu.h \
               $$PWD/QtnRibbonPageDsgnPlugin.h \
               $$PWD/QtnRibbonPlugins.h \
               $$PWD/QtnMainWindowDsgnPlugin.h \
               $$PWD/QtnMainWindowDsgnContainer.h \
               $$PWD/QtnMainWindowDsgnTaskMenu.h \
               $$PWD/QtnStatusBarDsgnPlugin.h \
               $$PWD/QtnCommandDsgn.h \
               $$PWD/QtnRibbonBackstageViewPlugin.h \
               $$PWD/QtnRibbonStyleDsgnPlugin.h \
               $$PWD/QtnRibbonGroupDsgnPlugin.h \
#			   $$PWD/QtnOfficeOptionsDialogPlugin.h

SOURCES     += $$PWD/QtnRibbonDsgnPlugin.cpp \
               $$PWD/QtnRibbonDsgnContainer.cpp \
               $$PWD/QtnRibbonDsgnTaskMenu.cpp \
               $$PWD/QtnRibbonPageDsgnPlugin.cpp \
               $$PWD/QtnRibbonPlugins.cpp \
               $$PWD/QtnMainWindowDsgnPlugin.cpp \
               $$PWD/QtnMainWindowDsgnContainer.cpp \
               $$PWD/QtnMainWindowDsgnTaskMenu.cpp \
               $$PWD/QtnStatusBarDsgnPlugin.cpp \
               $$PWD/QtnCommandDsgn.cpp \
               $$PWD/QtnRibbonBackstageViewPlugin.cpp \
               $$PWD/QtnRibbonStyleDsgnPlugin.cpp \
               $$PWD/QtnRibbonGroupDsgnPlugin.cpp \
#			   $$PWD/QtnOfficeOptionsDialogPlugin.cpp

RESOURCES += \
    $$PWD/QtnRibbonDsgnResources.qrc

CONFIG += plugin

!lessThan(QT_VER_MAJ, 5) {
  QT += widgets designer core-private widgets-private gui-private designer
} else {
  CONFIG      += designer
  DLLDESTDIR = $$PWD/../../bin
  DLLDESTDIR = $$member(DLLDESTDIR, 0)$$QTITAN_LIB_PREFIX
}

DEFINES += QTITAN_LIBRARY_STATIC
DEFINES += QTITAN_DESIGNER


