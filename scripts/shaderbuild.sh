#!/bin/bash
shader_dir="../shaders"
mkdir -p ../shaders/spiv
find "$shader_dir" -type f \( -name "*.vert" -o -name "*.frag" -o -name "*.comp" \) -print0 | while IFS= read -r -d '' shader_file; do
    glslangValidator -e main -gVS -V "$shader_file" -o "${shader_dir}/spiv/$(basename "$shader_file").spv" -g
done

