#!/bin/bash

./compile.sh
./build.sh
./copy-shaders.sh
build/$1
