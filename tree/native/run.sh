#!/bin/bash

# actual python script to be run is first bash argument in $1
cmd=$1

# get the path to this script (run.sh)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
#echo $DIR

# setup environment variables

export LD_LIBRARY_PATH=$DIR/../../lib:$LD_LIBRARY_PATH
export GI_TYPELIB_PATH=$DIR/../../lib:$GI_TYPELIB_PATH

# allows the python package under ./pygtk to be found
export PYTHONPATH=$DIR/../..:$PYTHONPATH

#echo $LD_LIBRARY_PATH
#echo $GI_TYPELIB_PATH
#echo $PYTHONPATH

build/webkitapp

