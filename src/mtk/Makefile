
PWD = $(shell pwd)
DEBUG = -g 
release : override DEBUG = -O3

DEFAULT_OPTS = -std=c++17 $(PG) -fPIC -shared 

ifneq ($(CXX),g++)
DEFAULT_OPTS = $(TS) -std=c++17 -stdlib=libc++ -fcoroutines-ts -D_RESUMABLE_FUNCTIONS_SUPPORTED 
endif

PKG_INC=$(shell pkg-config gobject-2.0 gtk+-3.0 gobject-introspection-1.0 webkit2gtk-4.0 gtksourceview-4 jsoncpp --cflags)
PKG_LIB=$(shell pkg-config gobject-2.0 gtk+-3.0 gobject-introspection-1.0 webkit2gtk-4.0 gtksourceview-4 jsoncpp --libs)

OPTIONS = -fpic -Wno-write-strings -pthread -D_REENTRANT $(DEFAULT_OPTS)

# compile flags
CPPFLAGS = -Wall -I ../../include $(DEBUG) $(INCLUDE) $(OPTIONS) $(PKG_INC)

#################################################
# source and build path information
#################################################

# the shared lib
NAME = libmtk.so
LIB = ./build/$(NAME)
release : override LIB = ./build/$(NAME)

LIB_SRC = .
LIB_BUILD = ./build


# Objects for the shared library
LIB_SRCFILESABS = $(shell ls $(LIB_SRC)/*.cpp)
LIB_SRCFILES =  $(notdir $(LIB_SRCFILESABS))
LIB_OBJFILES = $(LIB_SRCFILES:%.cpp=$(LIB_BUILD)/%.o)

# output targets
NAMESPACE=Mtk
NSVERSION=0.1
GIR_FILE=$(NAMESPACE)-$(NSVERSION).gir
TYPELIB_FILE=$(NAMESPACE)-$(NSVERSION).typelib


# headers to scan with g-ir
GIR_HEADERS= ../../include/mtk/mtkfile.h ../../include/mtk/mtkfiletree.h ../../include/mtk/mtkgit.h ../../include/mtk/mtkwebview.h ../../include/mtk/mtkeditor.h
GIR_SOURCES= mtkfile.cpp mtkfiletree.cpp mtkgit.cpp mtkwebview.cpp mtkeditor.cpp


#################################################
# usually no need to change anything below

#################################################

.SUFFIXES: .cpp

#################################################
# rule to compile all (default rule)
#################################################

all: $(LIB) $(LIB_BUILD)/$(TYPELIB_FILE)

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
	-bash -c "cd build && ln -s $(NAME) $(NAME).0"


$(LIB_BUILD)/$(TYPELIB_FILE): $(LIB_BUILD)/$(GIR_FILE)
	g-ir-compiler $(LIB_BUILD)/$(GIR_FILE) --output=$(LIB_BUILD)/$(TYPELIB_FILE)

# ../include/mtkwebview.h
# mtkwebview.cpp

$(LIB_BUILD)/$(GIR_FILE): $(GIR_HEADERS) $(GIR_SOURCES)
	libtool exec g-ir-scanner $^ -L $(LIB_BUILD) --library=mtk --warn-all --include=GtkSource-4 --include=WebKit2-4.0 --include=Gtk-3.0 --include=GObject-2.0  --namespace=$(NAMESPACE) --nsversion=$(NSVERSION) --output=$@
		
#################################################
# make clean
#################################################

clean:
	-find -name "*~" -exec rm {} \;
	-rm -f $(LIB_BUILD)/*.so
	-rm -f $(LIB_BUILD)/*.o
	-rm -f $(LIB_BUILD)/*.gir
	-rm -f $(LIB_BUILD)/*.typelib
	-rm -f $(LIB_BUILD)/*.so.0
	-rm -rf tmp-*

