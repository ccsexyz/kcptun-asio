#!/bin/bash
N=1
if [ $# -gt 0  ]; then
    N=$1
fi
git submodule update --init --recursive 

cd cryptopp 
make clean 
make libcryptopp.a "-j$N"
cd .. 

cd snappy 
rm CMakeCache.txt
cmake .
make clean 
make "-j$N"
cd ..

rm CMakeCache.txt 
cmake .
make clean 
make "-j$N"

