.RECIPEPREFIX := >

CXX=g++

TARGET=Rotozoomer.exe
SRC=Rotozoomer.cpp

CXXFLAGS=-O2 -Wall
LDFLAGS=-lfreeglut -lopengl32 -lglu32 -lm

all:
>$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
>cmd /C if exist $(TARGET) del /Q $(TARGET)