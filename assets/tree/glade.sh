#!/bin/bash

# get the path to this script (run.sh)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export GI_TYPELIB_PATH=$DIR/build
export GLADE_MODULE_SEARCH_PATH=$DIR/build
export GLADE_CATALOG_SEARCH_PATH=$DIR/glade 
export PYTHONPATH=$DIR/.. 

glade $1




