cmake_minimum_required(VERSION 3.12)
enable_testing()
find_package(GTest CONFIG REQUIRED)


add_executable(test_lvdfile)
target_sources(test_lvdfile PRIVATE "test_lvdwr.cpp")
target_link_libraries(test_lvdfile vmcore lvdfilereader)
target_link_libraries(test_lvdfile GTest::gtest_main GTest::gtest GTest::gmock GTest::gmock_main)
target_include_directories(test_lvdfile PRIVATE "${CMAKE_SOURCE_DIR}/src/plugins")
target_include_directories(test_lvdfile PRIVATE "${CMAKE_SOURCE_DIR}/include")

include(GoogleTest)
gtest_add_tests(test_lvdfile "" AUTO)
install(TARGETS test_lvdfile LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")

add_executable(test_mortoncode)
target_sources(test_mortoncode PRIVATE "test_mortoncode.cpp")
target_link_libraries(test_mortoncode vmcore)
target_link_libraries(test_mortoncode GTest::gtest_main GTest::gtest GTest::gmock GTest::gmock_main)
target_include_directories(test_mortoncode PRIVATE "${CMAKE_SOURCE_DIR}/src/plugins")
target_include_directories(test_mortoncode PRIVATE "${CMAKE_SOURCE_DIR}/include")

gtest_add_tests(test_mortoncode "" AUTO)
install(TARGETS test_mortoncode LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")


install(TARGETS vmcore LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
install(TARGETS lvdfilereader LIBRARY DESTINATION "lib" RUNTIME DESTINATION "bin" ARCHIVE DESTINATION "lib")
