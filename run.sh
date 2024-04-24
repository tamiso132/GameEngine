#!/usr/bin/env bash
#!/bin/bash

function print_help(){
    local self_name=$(basename "$0")
    echo ""
    echo "Usage:"
    echo "$self_name [application_option] <build_options> [<applications_specific_commands>]"
    echo ""

    echo "Application Options"
    echo "0:    GameEngine"
    echo "1:    MaterialEditor"
    echo "2:    AtlasGenerator"
    
    echo ""
    
    echo "Build Options"
    echo "r:    Release mode, default is debug mode"
    echo "b:    builds the application only"
   
    echo ""
   
    echo "Atlas Generator Commands"
    echo "l:    Length and height for the texture atlas"
    echo "dir:  Directory path for the images"
    echo ""
}

function build_shaders(){
    shader_dir="shaders"
    mkdir -p shaders/spiv
    find "$shader_dir" -type f \( -name "*.vert" -o -name "*.frag" -o -name "*.comp" \) -print0 | while IFS= read -r -d '' shader_file; do
        glslangValidator -e main -gVS -V "$shader_file" -o "${shader_dir}/spiv/$(basename "$shader_file").spv" -g
    done
}

function build_engine(){
    buildType=$1
    tracy=$2
  
    build_shaders
    echo $buildType
    mkdir -p build && cd build
    cmake -G Ninja -DPROFILE=$tracy -DCMAKE_BUILD_TYPE=$buildType -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
    ninja
  
}

function build_editor(){
    buildType=$1

    build_shaders
    cd tools/MaterialEditor
    mkdir -p build && cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=$buildType -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
    ninja
}

function build_atlas(){
    buildType=$1
    cd ../tools/AtlasGenerator/build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=$buildType -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ..
    ninja
}

function run_editor(){
    ./MaterialEditor
}

function run_engine(){
   ./VulkanGuide
}

function run_atlas(){
    
    textureAtlasLength=128
    imageDirectory=''

    while getopts "g:dir:" arg
    do
        case "${arg}" in
            l) textureAtlasLength=$OPTARG;;
            dir) imageDirectory=$OPTARG;;
        esac
    done


    cd ../tools/AtlasGenerator/build
    ./AtlasGenerator $imageDirectory $textureAtlasLength
}

function run_profiler(){

   ./../thirdparty/tracy/profiler/build/unix/Tracy-release&
}
#Start of Script!

buildType='Debug'
run=true
applicationType=$1
tracyProfile=false


shift
while getopts "hrbp" arg
do
    case "${arg}" in
        r) buildType='Release';;
        b)
            run=false
        ;;
        h) 
            print_help
            exit
            ;;
        p) 
            tracyProfile=true
            ;;
    esac
done

case "${applicationType}" in
    0)  
    echo $buildType
        build_engine $buildType $tracyProfile
        if ($tracyProfile); then
        run_profiler
        run_engine
        else
            if ($run); then
                run_engine
            fi
        fi
        ;;
        
    1) 
        # run editor
        build_editor $buildType
        if ($run); then
            run_editor
        fi
    ;; 
    2) 
        # run atlas
        build_atlas $buildType
        if (run); then
            run_atlas
        fi
    ;;
esac
