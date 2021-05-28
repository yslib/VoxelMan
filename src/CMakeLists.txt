cmake_minimum_required(VERSION 3.12)
find_package(SDL2 CONFIG REQUIRED)

aux_source_directory(. SRC)
add_subdirectory(plugins)

add_executable(cpurender)
target_sources(cpurender PRIVATE ${SRC})
if(WIN32)
target_link_libraries(cpurender vmcore SDL2::SDL2 SDL2::SDL2main)
else()
target_link_libraries(cpurender vmcore dl SDL2::SDL2 SDL2::SDL2main)
endif()
target_include_directories(cpurender PRIVATE "${CMAKE_SOURCE_DIR}/include" ${glfw_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

install(TARGETS vmcore LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
install(TARGETS cpurender LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
