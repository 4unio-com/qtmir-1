TARGET = mirapplicationplugin
TEMPLATE = lib

QT       += core quick dbus
QT       += qml-private core-private
QT       += gui-private # annoyingly needed by included NativeInterface
CONFIG   += link_pkgconfig plugin no_keywords

# CONFIG += c++11 # only enables C++0x
QMAKE_CXXFLAGS = -std=c++11 -fvisibility=hidden -fvisibility-inlines-hidden
QMAKE_CXXFLAGS_RELEASE += -Werror     # so no stop on warning in debug builds
QMAKE_LFLAGS = -std=c++11 -Wl,-no-undefined

PKGCONFIG += mircommon mirclient mirserver glib-2.0 process-cpp upstart-app-launch-2

INCLUDEPATH += ../../../platforms/mirserver
LIBS += -L../../../platforms/mirserver -lqpa-mirserver
QMAKE_RPATHDIR += $$[QT_INSTALL_PLUGINS]/platforms # where libqpa-mirserver.so is installed

TARGET = $$qtLibraryTarget($$TARGET)
uri = Mir.Application

SOURCES += application_manager.cpp \
    application.cpp \
    debughelpers.cpp \
    desktopfilereader.cpp \
    plugin.cpp \
    applicationscreenshotprovider.cpp \
    dbuswindowstack.cpp \
    taskcontroller.cpp \
    mirsurfacemanager.cpp \
    ubuntukeyboardinfo.cpp \
    mirsurfaceitem.cpp \
    mirbuffersgtexture.cpp \
    processcontroller.cpp \
    proc_info.cpp \
    upstart/applicationcontroller.cpp

HEADERS += application_manager.h \
    applicationcontroller.h \
    application.h \
    debughelpers.h \
    desktopfilereader.h \
    applicationscreenshotprovider.h \
    dbuswindowstack.h \
    taskcontroller.h \
    mirsurfacemanager.h \
    ubuntukeyboardinfo.h \
    /usr/include/unity/shell/application/ApplicationManagerInterface.h \
    /usr/include/unity/shell/application/ApplicationInfoInterface.h \
    mirsurfaceitem.h \
    mirbuffersgtexture.h \
    processcontroller.h \
    proc_info.h \
    upstart/applicationcontroller.h

installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)

QML_FILES = qmldir
qml_files.path = $$installPath
qml_files.files = $$QML_FILES

target.path = $$installPath

INSTALLS += target qml_files
