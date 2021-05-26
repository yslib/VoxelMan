
cmake_minimum_required(VERSION 3.12)

aux_source_directory(. SRC)

find_package(glfw3 CONFIG REQUIRED)

add_subdirectory(plugins)

add_executable(volvis)
target_sources(volvis PRIVATE "../gl3w/GL/gl3w.c" "main.cpp")
if(WIN32)
target_link_libraries(volvis glfw vmcore)
else()
target_link_libraries(volvis glfw vmcore dl)
endif()
target_include_directories(volvis PRIVATE "../include" "../gl3w" ${glfw_INCLUDE_DIRS})

find_package(SDL2 CONFIG REQUIRED)

add_executable(cpurender)
target_sources(cpurender PRIVATE "../gl3w/GL/gl3w.c" "cpurenderer.cpp")
if(WIN32)
target_link_libraries(cpurender glfw vmcore SDL2::SDL2)
else()
target_link_libraries(cpurender glfw vmcore dl SDL2::SDL2)
endif()
target_include_directories(cpurender PRIVATE "../include" "../gl3w" ${glfw_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

install(TARGETS volvis LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
install(TARGETS vmcore LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
install(TARGETS cpurender LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
