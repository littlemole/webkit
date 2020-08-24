
DEBUG = -g 
release : override DEBUG = -O3

DEFAULT_OPTS = -std=c++17 $(PG) -fPIC  

ifneq ($(CXX),g++)
DEFAULT_OPTS = $(TS) -std=c++17 -stdlib=libc++ -fcoroutines-ts -D_RESUMABLE_FUNCTIONS_SUPPORTED 
endif

PKG_INC=$(shell pkg-config gobject-2.0 gdk-3.0 webkit2gtk-4.0 jsoncpp --cflags)
PKG_LIB=$(shell pkg-config gobject-2.0 gdk-3.0 webkit2gtk-4.0 jsoncpp --libs)

OPTIONS = -fpic -Wno-write-strings -pthread -D_REENTRANT $(DEFAULT_OPTS)

# compile flags
CPPFLAGS = -Wall -I../../include $(DEBUG) $(INCLUDE) $(OPTIONS) $(PKG_INC)

LIBS = -L../build -lmtk
#################################################
# source and build path information
#################################################

# the shared lib
BIN = ./build/webkitapp
release : override LIB = ./build/webkitapp

BIN_SRC = .
BIN_BUILD = ./build


# Objects for the shared library
BIN_SRCFILESABS = $(shell ls $(BIN_SRC)/*.cpp)
BIN_SRCFILES =  $(notdir $(BIN_SRCFILESABS))
BIN_OBJFILES = $(BIN_SRCFILES:%.cpp=$(BIN_BUILD)/%.o)

#################################################
# usually no need to change anything below
#################################################

.SUFFIXES: .cpp

#################################################
# rule to compile all (default rule)
#################################################

all: $(BIN) 

#################################################
# release rule 
#################################################

release: all

#################################################
# actual build rules
#################################################
	
#################################################
# rules to compile .o files from .cpp source
#################################################

$(BIN_BUILD)/%.o: $(BIN_SRC)/%.cpp 
	-mkdir -p $(BIN_BUILD)
	$(CXX) -c $^ -o $@ $(CPPFLAGS)

#################################################
# rule to compile the lib file from .o files
#################################################

$(BIN): $(BIN_OBJFILES)
	$(CXX)  -o $(BIN) $(BIN_OBJFILES) $(LIBS) $(CPPFLAGS) $(PKG_LIB)

		
#################################################
# make clean
#################################################

clean:
	-find -name "*~" -exec rm {} \;
	-rm -f $(BIN)
	-rm -f $(BIN_BUILD)/*.o
	
