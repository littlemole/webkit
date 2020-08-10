#!/bin/bash

# get the path to this script (run.sh)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

GLADE_CATALOG_SEARCH_PATH=$DIR/glade PYTHONPATH=$DIR glade $1



