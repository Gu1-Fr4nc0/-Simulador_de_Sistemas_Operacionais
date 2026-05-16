# Makefile — Simulador de SO
# Requer Qt5 ou Qt6 instalado.
# Uso:  make          → compila GUI (Qt)
#       make cli      → compila modo terminal (sem Qt)
#       make clean    → limpa artefatos

CXX      = g++
CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11   -Iinclude
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude

# Detecta Qt automaticamente
QTVER   := $(shell qmake -query QT_VERSION 2>/dev/null | cut -d. -f1)
ifeq ($(QTVER),)
    $(warning Qt não encontrado. Use 'make cli' para compilar sem GUI.)
    QTVER = 0
endif

ifeq ($(QTVER),6)
    QT_CFLAGS  := $(shell pkg-config --cflags Qt6Widgets)
    QT_LIBS    := $(shell pkg-config --libs   Qt6Widgets)
    MOC        := moc
else
    QT_CFLAGS  := $(shell pkg-config --cflags Qt5Widgets)
    QT_LIBS    := $(shell pkg-config --libs   Qt5Widgets)
    MOC        := moc
endif

TARGET  = so_simulator
OBJDIR  = build

C_SRCS  = src/process.c src/scheduler.c src/memory.c src/main.c
CPP_SRCS= src/mainwindow.cpp

C_OBJS   = $(patsubst src/%.c,   $(OBJDIR)/%.o, $(C_SRCS))
CPP_OBJS = $(patsubst src/%.cpp, $(OBJDIR)/%.o, $(CPP_SRCS))

MOC_OUT  = $(OBJDIR)/mainwindow.moc

# ── Regra padrão: GUI ───────────────────────────────────────────────
all: $(OBJDIR) $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $^ $(QT_LIBS) -o $@
	@echo "✔  Binário gerado: $(TARGET)"

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(QT_CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: src/%.cpp $(MOC_OUT) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -I$(OBJDIR) -c $< -o $@

$(MOC_OUT): src/mainwindow.cpp | $(OBJDIR)
	$(MOC) $(QT_CFLAGS) $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

# ── Modo CLI (sem Qt) ───────────────────────────────────────────────
CLI_SRCS = src/process.c src/scheduler.c src/memory.c src/main.c
CLI_OBJS = $(patsubst src/%.c, $(OBJDIR)/cli_%.o, $(CLI_SRCS))

cli: $(OBJDIR) $(CLI_OBJS)
	$(CC) $(CLI_OBJS) -o $(TARGET)_cli
	@echo "✔  Binário CLI gerado: $(TARGET)_cli"

$(OBJDIR)/cli_%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -DCLI_MODE -c $< -o $@

# ── Limpeza ─────────────────────────────────────────────────────────
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET)_cli
	@echo "✔  Limpeza concluída."

.PHONY: all cli clean
