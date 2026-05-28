CXX=clang++
TARGET=Rotozoomer
SRC=Rotozoomer.cpp

all:
	$(CXX) $(SRC) -o $(TARGET) -std=c++17 -O2 -Wall -framework OpenGL -framework GLUT

clean:
	rm -f $(TARGET)