# c++ compiler
CC=g++

# build directories
INC = ./include
SRC = ./src
BUILD = ./build

# includes
C_INCLUDES= -I $(INC) `pkg-config --cflags gobject-2.0 webkitgtk-3.0 gobject-introspection-1.0 python-2.7`
CFLAGS=$(C_INCLUDES) -g -std=c++0x

# link dependencies
LIBS=`pkg-config --libs gobject-2.0 webkitgtk-3.0 gobject-introspection-1.0 python-2.7`
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
	libtool exec g-ir-scanner $^ --library=pywebkit $(C_INCLUDES) --include=WebKit-3.0 --include=GObject-2.0 --namespace=$(NAMESPACE) --nsversion=$(NSVERSION) --output=$@

$(BUILD)/%.lo: $(SRC)/%.cpp 
	libtool compile $(CC) $(CFLAGS) -c $< -o $@
	

clean:
	-rm $(BUILD)/*.lo libpywebkit.la $(TYPELIB_FILE) $(GIR_FILE)
	-rm $(BUILD)/*.o *.lo
	-rm *~
	-rm -rf .libs


