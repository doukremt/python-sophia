#!/usr/bin/env bash

set -o errexit

prefix="/usr/lib"
sophia_repo='https://github.com/pmwkaa/sophia.git'

mkdir -p lib
cd lib
if [ -d "sophia" ]; then
    cd sophia
    git fetch $sophia_repo
else
    git clone $sophia_repo
    cd sophia
fi

make
sudo cp -v db/libsophia.a "$prefix"
for file in db/libsophia.so*; do
	sudo cp -v $file "$prefix"
done
sudo cp -v db/sophia.h /usr/include
make clean
