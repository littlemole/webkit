#!/bin/bash

# source me!

PWD=$(pwd)
echo $PWD
export LD_LIBRARY_PATH=$PWD/.libs:$LD_LIBRARY_PATH
export GI_TYPELIB_PATH=$PWD
export PYTHONPATH=$PWD/py/build
echo $LD_LIBRARY_PATH
echo $GI_TYPELIB_PATH
echo $PYTHONPATH

