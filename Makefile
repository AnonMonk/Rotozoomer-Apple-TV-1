# =========================
# Projekt
# =========================

SRC = Rotozoomer.cpp TunnelEffect.cpp
NAME = Rotozoomer

# Standardziel, falls kein TARGET angegeben wird
TARGET ?= MAC

# =========================
# Gemeinsame Optionen
# =========================

COMMON_WARNINGS = -Wall

# =========================
# Windows / MinGW
# Aufruf:
# mingw32-make TARGET=WINDOWS
# oder:
# make TARGET=WINDOWS
# =========================

ifeq ($(TARGET),WINDOWS)

CXX = g++
OUT = $(NAME).exe
CXXFLAGS = -O2 $(COMMON_WARNINGS) -std=c++17
LDFLAGS = -lfreeglut -lopengl32 -lglu32

endif

# =========================
# Moderner Mac
# macOS 26 / Apple Silicon / Intel
# Aufruf:
# make TARGET=MAC
# =========================

ifeq ($(TARGET),MAC)

CXX = clang++
OUT = $(NAME)
CXXFLAGS = -O2 $(COMMON_WARNINGS) -std=c++17
LDFLAGS = -framework OpenGL -framework GLUT

endif

# =========================
# Apple TV 1
# Mac OS X 10.4 / 10.5
# Aufruf:
# make TARGET=APPLETV
#
# Wichtig:
# Kein -std=c++17 hier, weil der alte Apple-TV-Compiler
# das sehr wahrscheinlich nicht kann.
# =========================

ifeq ($(TARGET),APPLETV)

CXX = g++
OUT = $(NAME)
CXXFLAGS = -O2 $(COMMON_WARNINGS) -DSTBI_NO_THREAD_LOCALS
LDFLAGS = -framework OpenGL -framework GLUT

endif

# =========================
# Sicherheitsprüfung
# =========================

ifndef CXX
$(error Ungueltiges TARGET. Benutze TARGET=WINDOWS oder TARGET=MAC oder TARGET=APPLETV)
endif

# =========================
# Build-Regeln
# =========================

all:
	$(CXX) $(SRC) -o $(OUT) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(NAME) $(NAME).exe

info:
	@echo TARGET = $(TARGET)
	@echo CXX = $(CXX)
	@echo OUT = $(OUT)
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
