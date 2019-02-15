cmake_minimum_required (VERSION 2.6)
project (delegate)

link_directories(/usr/src/gtest)

add_executable(delegate_11 test/delegate_test.cpp)
add_executable(delegate_14 test/delegate_test.cpp)
add_executable(delegate_17 test/delegate_test.cpp)

set_property(TARGET delegate_11 PROPERTY CXX_STANDARD 11)
set_property(TARGET delegate_14 PROPERTY CXX_STANDARD 14)
set_property(TARGET delegate_17 PROPERTY CXX_STANDARD 17)

target_include_directories(delegate_11 PUBLIC include)
target_include_directories(delegate_14 PUBLIC include)
target_include_directories(delegate_17 PUBLIC include)

target_link_libraries(delegate_11 PUBLIC gtest gtest_main pthread)
target_link_libraries(delegate_14 PUBLIC gtest gtest_main pthread)
target_link_libraries(delegate_17 PUBLIC gtest gtest_main pthread)
