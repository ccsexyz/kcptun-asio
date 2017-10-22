#!/bin/bash

cd snappy  
git checkout . && git clean -xdf
cd .. 

cd cryptopp 
git checkout . && git clean -xdf 
cd .. 

make clean  
rm CMakeCache.txt  