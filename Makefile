all: run_test

INC_FLAGS:= -Iinclude -I/usr/src/gtest/include
LIB_FLAGS:= -L/usr/src/gtest -lgtest -lgtest_main

delegate_test.out: test/delegate_test.cpp include/delegate/delegate.hpp
	g++ $(INC_FLAGS) -o delegate_test.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread

run_test: delegate_test.out
	./delegate_test.out

.PHONY: format
format:
	clang-format-6.0 -i include/delegate/delegate.hpp
	clang-format-6.0 -i test/delegate_test.cpp
