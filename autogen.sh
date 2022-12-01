#!/bin/sh
rm -rf interflop-stdlib
git submodule update --init --recursive
mkdir -p m4
./install-stdlib.sh 2>&1 >install_stdlib.log
if [ $? != 0 ]; then
    echo "Error while installing interflop-stdlib"
    cat install_stdlib.log
    exit 1
else
    echo "Installing 'interflop-stdlib'"
fi
autoreconf -is
