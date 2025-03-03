#!/bin/bash

set -euo pipefail

dir=build

[ ! -d $dir ] && mkdir $dir
cd $dir
[ ! -f Makefile ] && qmake ../

make
cd ..
