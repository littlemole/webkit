#!/bin/bash

# actual python script to be run is first bash argument in $1
cmd=$1

# get the path to this script (run.sh)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
#echo $DIR

# setup environment variables

# allows our custom GtkWidget libwebview.so file to be loaded
export LD_LIBRARY_PATH=$DIR/webkit/build:$LD_LIBRARY_PATH

# allows Gnome Introspection (gi) to find our custom GtkWidget via gir/typelib
# allows gi repository to find our Pywebkit-0.1.typelib
export GI_TYPELIB_PATH=$DIR/webkit/build:$GI_TYPELIB_PATH

# allows the python package under ./pygtk to be found
export PYTHONPATH=$DIR:$PYTHONPATH

#echo $LD_LIBRARY_PATH
#echo $GI_TYPELIB_PATH
#echo $PYTHONPATH

# run the python script
/usr/bin/python3 $cmd

