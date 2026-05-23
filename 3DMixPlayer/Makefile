# Makefile - 3DMixPlayer (standalone, native Haiku OS only)
#
# Builds a self-contained BeOS 3dmix player & 3D visualizer extracted
# from VeniceDAW. Compile and run on real Haiku systems only.

CXX      = g++
CXXFLAGS = -Wall -Wno-multichar -std=c++17 -pthread -O2 -fPIC
INCLUDES = -Isrc -I/boot/system/develop/headers -I/boot/system/develop/headers/cpp
LIBS     = -lbe -lroot -lGL -lGLU -lmedia -ltracker -ltranslation

TARGET = 3DMixPlayer

SOURCES = \
	src/3DMixPlayer.cpp \
	src/audio/AudioLogging.cpp \
	src/audio/BiquadFilter.cpp \
	src/audio/3dmix/3DMixFormat.cpp \
	src/audio/3dmix/3DMixParser.cpp \
	src/audio/3dmix/CoordinateSystemMapper.cpp \
	src/audio/3dmix/AudioPathResolver.cpp

OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean install run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) $(LIBS) -o $(TARGET)
	@echo "Build complete: ./$(TARGET) <project.3dmix>"

%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned."

install: $(TARGET)
	cp $(TARGET) /boot/home/Desktop/$(TARGET)
	@echo "Installed to Desktop."

run: $(TARGET)
	./$(TARGET) $(FILE)

# Debug build with symbols and verbose logging
debug: CXXFLAGS += -g -DDEBUG
debug: clean all
