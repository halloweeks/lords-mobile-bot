#!/bin/sh

set -e

clear

if [ ! -d build ]; then
    mkdir build
fi

cd build

cmake .. -DDEBUG_LOG=OFF
make -j$(nproc)

cp ./client $HOME/
chmod +x $HOME/client