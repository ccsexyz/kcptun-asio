#!/bin/bash

cd gflags
git checkout . && git clean -xdf
cd ..

cd snappy  
git checkout . && git clean -xdf
cd .. 

cd cryptopp 
git checkout . && git clean -xdf 
cd ..

cd glog
git checkout . && git clean -xdf
cd ..

cd kcp
git checkout . && git clean -xdf
cd ..

make clean  
rm CMakeCache.txt
rm *.cmake
