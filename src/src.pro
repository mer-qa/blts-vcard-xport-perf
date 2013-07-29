include(../common.pri)

TEMPLATE = app
TARGET = blts-vcard-xport-perf
QT -= gui
QT += contacts versit

CONFIG += link_pkgconfig
PKGCONFIG += bltscommon

HEADERS = \
    blts-vcard-xport-perf.h \

SOURCES = \
    blts-vcard-xport-perf.cpp \
    cli.c \

target.path = $${TESTS_DIR}
INSTALLS += target
