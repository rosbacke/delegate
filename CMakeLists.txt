cmake_minimum_required (VERSION 3.0)

project (delegate)

add_library(delegate INTERFACE)

target_sources(delegate INTERFACE ${CMAKE_SOURCE_DIR}/include/delegate/delegate.hpp)

target_include_directories(delegate INTERFACE include/)


# Testing

set(PICKY_FLAGS -Wall -Wextra  -Wstrict-aliasing -pedantic -Werror -Wunreachable-code -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo  -Wstrict-overflow=5 -Wswitch-default -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option
)

enable_testing()

# Note: If you have trouble locating gtest, try:
# export GTEST_ROOT=<path>
# In Ubuntu, gtest source is installed under /usr/src/gtest.
# build it and set GTEST_ROOT to the build folder.
find_package(GTest REQUIRED)

add_test(del_11 delegate_11)
add_test(del_14 delegate_14)
add_test(del_17 delegate_17)

add_executable(delegate_11 test/delegate_test.cpp)
add_executable(delegate_14 test/delegate_test.cpp)
add_executable(delegate_17 test/delegate_test.cpp)

target_compile_options(delegate_11 PRIVATE -std=c++11 ${PICKY_FLAGS}) 
target_compile_options(delegate_14 PRIVATE -std=c++14 ${PICKY_FLAGS}) 
target_compile_options(delegate_17 PRIVATE -std=c++17 ${PICKY_FLAGS}) 

# The following is normally the recommended way, but will give gnu extensions.
#set_property(TARGET delegate_11 PROPERTY CXX_STANDARD 11)
#set_property(TARGET delegate_14 PROPERTY CXX_STANDARD 14)
#set_property(TARGET delegate_17 PROPERTY CXX_STANDARD 17)

target_link_libraries(delegate_11 PRIVATE delegate GTest::GTest GTest::Main)
target_link_libraries(delegate_14 PRIVATE delegate GTest::GTest GTest::Main)
target_link_libraries(delegate_17 PRIVATE delegate GTest::GTest GTest::Main)
