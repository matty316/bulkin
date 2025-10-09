#!/bin/bash

cd shaders

/home/matty/vulkan-sdk/default/x86_64/bin/glslc shader.vert -o vert.spv
/home/matty/vulkan-sdk/default/x86_64/bin/glslc shader.frag -o frag.spv

cd ..

VERT_BINARY_FILE="shaders/vert.spv"
FRAG_BINARY_FILE="shaders/frag.spv"
VERT_CPP_HEADER="include/vert-shader.h"
FRAG_CPP_HEADER="include/frag-shader.h"

# Convert the binary file to a C-style byte array and save it to the header file
xxd -i "$VERT_BINARY_FILE" >"$VERT_CPP_HEADER"
xxd -i "$FRAG_BINARY_FILE" >"$FRAG_CPP_HEADER"
