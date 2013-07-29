include(common.pri)

TEMPLATE = subdirs
SUBDIRS = src

tests_xml.files = tests.xml
tests_xml.path = $${TESTS_DIR}
INSTALLS += tests_xml
