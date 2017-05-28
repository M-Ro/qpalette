OBJ=colormap.o bmp.o png.o qpalette.o
ICON_OBJ=icon.res

TARGET=qpalette
LDFLAGS=-Wl,-Bstatic -lpng -lz
CXX=gcc
LD=gcc
CXXFLAGS=--Wall -Wextra -Wno-comment

all: $(TARGET)

qpalette: $(OBJ) $(ICON_OBJ)
	$(LD) $(OBJ) $(ICON_OBJ) -o $(TARGET) $(LDFLAGS)
	
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