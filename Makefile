CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

SRC = src/main.cpp src/schema.cpp src/list.cpp src/hashtable.cpp src/document.cpp src/jsonparser.cpp src/storage.cpp src/commandparser.cpp
OBJ = $(SRC:.cpp=.o)
BIN = dbms

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $(BIN)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)
