QT       += core gui widgets

TARGET   = so_simulator
TEMPLATE = app
CONFIG   += c++17

# Fontes C
SOURCES += \
    src/main.c \
    src/process.c \
    src/scheduler.c \
    src/memory.c

# Fonte C++ (Qt GUI)
SOURCES += src/mainwindow.cpp

HEADERS += \
    include/process.h \
    include/scheduler.h \
    include/memory.h \
    include/mainwindow.h

INCLUDEPATH += include

# Descomente para compilar modo CLI (sem Qt)
# DEFINES += CLI_MODE
