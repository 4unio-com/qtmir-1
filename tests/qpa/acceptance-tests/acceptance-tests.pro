include(../../test-includes.pri)
include(../../common/common.pri)

TARGET = qpa_mir_server_and_client_acceptance_tests

INCLUDEPATH += \
    ../../../src/platforms/mirserver

SOURCES += \
    acceptance_tests.cpp \
    stub_graphics_platform.cpp \
    mir_test_framework/server_runner.cpp

#CONFIG += link_pkgconfig
#PKG_CONFIG += mirclient

# need to link in the QPA plugin too for access to MirServerConfiguration
LIBS += -Wl,-rpath,$${PWD}/../../../src/platforms/mirserver \
    -L../../../src/platforms/mirserver -lqpa-mirserver
