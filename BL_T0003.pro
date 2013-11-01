TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    src/utils.c \
    src/commands.c \
    src/cmd_def.c \
    src/main.cpp \
    src/simpleserial.cpp

OTHER_FILES += \
    README.md \
    LICENSE

HEADERS += \
    inc/utils.h \
    inc/config.h \
    inc/cmd_def.h \
    inc/apitypes.h \
    inc/simpleserial.h

# C++11
QMAKE_CXXFLAGS += -std=c++0x

# libraries
unix|win32: LIBS += -lpthread -lboost_system -lboost_regex
