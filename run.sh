#!/bin/bash
cmd=$1
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
#PWD=$(pwd)
PWD=$DIR
#echo $PWD
export LD_LIBRARY_PATH=$PWD/webkit/build:$LD_LIBRARY_PATH
export GI_TYPELIB_PATH=$PWD/webkit/build
export PYTHONPATH=$PWD
#echo $LD_LIBRARY_PATH
#echo $GI_TYPELIB_PATH
#echo $PYTHONPATH
/usr/bin/python3 $cmd

