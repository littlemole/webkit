#!/bin/bash

PWD=$(pwd)
echo $PWD
export LD_LIBRARY_PATH=$PWD/.libs:$LD_LIBRARY_PATH
export GI_TYPELIB_PATH=$PWD
echo $LD_LIBRARY_PATH
echo $GI_TYPELIB_PATH
/usr/bin/python3 curl.py