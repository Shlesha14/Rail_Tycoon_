# =========================================================================
#  Rail Empire  -  Makefile  (Windows + MinGW + SFML 3)
#
#  Usage:
#    make        -> builds game.exe
#    make clean  -> removes game.exe and all .o files
#
#  Edit SFML_DIR if your SFML is installed elsewhere.
# =========================================================================

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2
SFML_DIR = C:/TransportGame/SFML
INCLUDES = -I"$(SFML_DIR)/include"
LIBS     = -L"$(SFML_DIR)/lib" -lsfml-graphics -lsfml-window -lsfml-system

TARGET  = game.exe
SOURCES = main.cpp globals.cpp map.cpp algorithms.cpp routes.cpp ai.cpp renderer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "Build complete: $(TARGET)"

%.o: %.cpp rail_empire.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	del /Q $(OBJECTS) $(TARGET) 2>nul || rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
