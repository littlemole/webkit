# c++ compiler
CC=g++

# build directories
INC = ./include
SRC = ./src
BUILD = ./build

# includes
C_INCLUDES= -I $(INC) `pkg-config --cflags gobject-2.0 webkit2gtk-4.0 gobject-introspection-1.0 python3-embed`
CFLAGS=$(C_INCLUDES) -g -std=c++0x

# link dependencies
LIBS=`pkg-config --libs gobject-2.0 webkit2gtk-4.0 gobject-introspection-1.0 python3-embed`
LIBDIR=/usr/local/lib

# source files and objects to be build
SRCFILESABS = $(shell ls $(SRC)/*.cpp)
SOURCES =  $(notdir $(SRCFILESABS))
OBJECTS = $(SOURCES:%.cpp=$(BUILD)/%.lo)

# output targets
NAMESPACE=Py
NSVERSION=0.1
GIR_FILE=$(NAMESPACE)-$(NSVERSION).gir
TYPELIB_FILE=$(NAMESPACE)-$(NSVERSION).typelib

# build rules

all: libpywebkit.la $(TYPELIB_FILE)

libpywebkit.la: $(OBJECTS)	
	libtool link $(CC) $(LIBS) -rpath $(LIBDIR) $(OBJECTS) -o $@ 

$(TYPELIB_FILE): $(GIR_FILE)
	g-ir-compiler $(GIR_FILE) --output=$(TYPELIB_FILE)

$(GIR_FILE): $(INC)/pywebkit.h $(SRC)/pywebkit.cpp
	libtool exec g-ir-scanner $^ --library=pywebkit $(C_INCLUDES) --include=WebKit2-4.0 --include=GObject-2.0 --namespace=$(NAMESPACE) --nsversion=$(NSVERSION) --output=$@

$(BUILD)/%.lo: $(SRC)/%.cpp 
	libtool compile $(CC) $(CFLAGS) -c $< -o $@
	

clean:
	-rm $(BUILD)/*.lo libpywebkit.la $(TYPELIB_FILE) $(GIR_FILE)
	-rm $(BUILD)/*.o *.lo
	-rm *~
	-rm -rf .libs


proto:
	-rm build/gdbus.o
	g++ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -g -c gdbus.cpp -o build/gdbus.o
	g++ build/gdbus.o -lgio-2.0 -lgobject-2.0 -lglib-2.0 -o a.out
	./a.out 