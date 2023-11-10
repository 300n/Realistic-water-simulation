CXX = gcc
CXXFLAGS = -Wno-format -Wall $(shell sdl2-config --cflags) 
LDFLAGS = $(shell sdl2-config --libs)  -lSDL2_mixer -lSDL2 -lSDL2_ttf -lm
SRC = TIPE.c
OUTPUT = a.out

all: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) && ./$(OUTPUT)

clean:
	rm -f $(OUTPUT)







