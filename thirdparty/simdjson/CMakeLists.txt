
add_library(simdjson STATIC)

target_include_directories(simdjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(simdjson PRIVATE
    simdjson.cpp
    simdjson.h
)