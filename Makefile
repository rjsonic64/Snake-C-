CXX = g++
CXXFLAGS = -std=c++17 \
-Wall \
-Wextra

SRC = src/Snake.cpp

OUT = build/run

all : $(OUT)
$(OUT) : $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT)

clean :
	rm -f $(OUT)