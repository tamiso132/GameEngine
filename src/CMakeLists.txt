
# Define an initial list
macro(add_sources)
    # Get the relative path from the project root to the current source directory
    file(RELATIVE_PATH RELATIVE_PATH_TO_PROJECT ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Iterate over the source files and add them to the list
    foreach(SOURCE_FILE ${ARGV})
        # Append to the list of sources with the relative path
        list(APPEND SOURCES_FILES_GAME_ENGINE ${RELATIVE_PATH_TO_PROJECT}/${SOURCE_FILE})
    endforeach()
    set(SOURCES_FILES_GAME_ENGINE ${SOURCES_FILES_GAME_ENGINE} PARENT_SCOPE)
endmacro()

macro(include_this)
    file(RELATIVE_PATH RELATIVE_PATH_TO_PROJECT ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    list(APPEND INCLUDE_DIRECTORIES_ENGINE "\n${RELATIVE_PATH_TO_PROJECT}")
    set(INCLUDE_DIRECTORIES_ENGINE ${INCLUDE_DIRECTORIES_ENGINE} PARENT_SCOPE)
endmacro()




add_subdirectory(renderer)
add_subdirectory(input)
add_subdirectory(collision)
add_subdirectory(camera)

add_sources(
    main.cpp
)

include_this()

