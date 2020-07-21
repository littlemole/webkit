#!/bin/bash

PWD=$(pwd)
echo $PWD
export LD_LIBRARY_PATH=$PWD/webkit/build:$LD_LIBRARY_PATH
export GI_TYPELIB_PATH=$PWD/webkit/build
#export PYTHONPATH=$PWD/py/build
echo $LD_LIBRARY_PATH
echo $GI_TYPELIB_PATH
#echo $PYTHONPATH
/usr/bin/python3 curl.py
