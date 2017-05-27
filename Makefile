OBJ=colormap.o bmp.o png.o qpalette.o

TARGET=qpalette
LDFLAGS=-lpng
CXX=gcc
LD=gcc
CXXFLAGS=--Wall -Wextra -Wno-comment

all: $(TARGET)

qpalette: $(OBJ)
	$(LD) $(OBJ) -o $(TARGET) $(LDFLAGS)
	
debug: all
	CFLAGS="$(CFLAGS) -g -DDEBUG -Wall -Werror"

release: all
	CFLAGS="$(CFLAGS) -DRELEASE -Wall -Werror -O2"

%.o: %.cpp
	$(CXX) $< -o $@ -c $(CXXFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)

dist-clean: clean
	rm -f *~