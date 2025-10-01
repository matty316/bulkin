#!/bin/bash

cd shaders

/Users/matty/VulkanSDK/1.4.321.0/macOS/bin/glslc shader.vert -o vert.spv
/Users/matty/VulkanSDK/1.4.321.0/macOS/bin/glslc shader.frag -o frag.spv

cd ..

VERT_BINARY_FILE="shaders/vert.spv"
FRAG_BINARY_FILE="shaders/frag.spv"
VERT_CPP_HEADER="include/vert-shader.h"
FRAG_CPP_HEADER="include/frag-shader.h"

# Convert the binary file to a C-style byte array and save it to the header file
xxd -i "$VERT_BINARY_FILE" >"$VERT_CPP_HEADER"
xxd -i "$FRAG_BINARY_FILE" >"$FRAG_CPP_HEADER"
