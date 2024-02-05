find_package(simdjson)

add_library(fastgltf STATIC)

target_include_directories(fastgltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(fastgltf PRIVATE
    base64.cpp
    fastgltf.cpp
)

