
DEBUG = -g 
release : override DEBUG = -O3

DEFAULT_OPTS = -std=c++17 $(PG) -fPIC -shared 

ifneq ($(CXX),g++)
DEFAULT_OPTS = $(TS) -std=c++17 -stdlib=libc++ -fcoroutines-ts -D_RESUMABLE_FUNCTIONS_SUPPORTED 
endif

PKG_INC=$(shell pkg-config webkit2gtk-4.0 --cflags)
PKG_LIB=$(shell pkg-config webkit2gtk-4.0 --libs)

OPTIONS = -fpic -Wno-write-strings -pthread -D_REENTRANT $(DEFAULT_OPTS)

# compile flags
CPPFLAGS = -Wall -I../../include $(DEBUG) $(INCLUDE) $(OPTIONS) $(PKG_INC)


#################################################
# source and build path information
#################################################

# the shared lib
LIB = ./build/webkit2_web_extension.so
release : override LIB = ./build/webkit2_web_extension.so

LIB_SRC = .
LIB_BUILD = ./build


# Objects for the shared library
LIB_SRCFILESABS = $(shell ls $(LIB_SRC)/*.cpp)
LIB_SRCFILES =  $(notdir $(LIB_SRCFILESABS))
LIB_OBJFILES = $(LIB_SRCFILES:%.cpp=$(LIB_BUILD)/%.o)

#################################################
# usually no need to change anything below
#################################################

.SUFFIXES: .cpp

#################################################
# rule to compile all (default rule)
#################################################

all: $(LIB) 

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

$(LIB_BUILD)/%.o: $(LIB_SRC)/%.cpp 
	-mkdir -p $(LIB_BUILD)
	$(CXX) -c $^ -o $@ $(CPPFLAGS)

#################################################
# rule to compile the lib file from .o files
#################################################

$(LIB): $(LIB_OBJFILES)
	$(CXX)  -o $(LIB) $(LIB_OBJFILES)  $(CPPFLAGS) $(PKG_LIB)

		
#################################################
# make clean
#################################################

clean:
	-find -name "*~" -exec rm {} \;
	-rm -f $(LIB_BUILD)/*.so
	-rm -f $(LIB_BUILD)/*.o
	
