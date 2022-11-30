#!/bin/bash

cd interflop-stdlib
./autogen.sh
./configure --prefix=$(realpath install)
make
make install
cd ..
cp interflop-stdlib/ax_interflop_stdlib.m4 m4/
