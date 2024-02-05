file(GLOB BOOTSTRAP_SOURCE *.cpp)
file(GLOB BOOTSTRAP_HEADER *.h)

add_library(vk-bootstrap STATIC ${BOOTSTRAP_SOURCE} ${BOOTSTRAP_HEADER})

target_include_directories(vk-bootstrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})