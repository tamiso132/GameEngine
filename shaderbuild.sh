#!/bin/bash

# Specify the directory containing the shader files
shader_dir="shaders"
mkdir -p shaders/spiv
# Use find to get all GLSL files in the directory
find "$shader_dir" -type f \( -name "*.vert" -o -name "*.frag" -o -name "*.comp" \) -print0 | while IFS= read -r -d '' shader_file; do
    # Print the file being processed (optional)
    echo "Processing $shader_file"
    glslangValidator -e main -gVS -V "$shader_file" -o "${shader_dir}/spiv/$(basename "$shader_file").spv" -g
done

