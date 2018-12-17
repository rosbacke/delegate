all: run_test

INC_FLAGS:= -Iinclude -I/usr/src/gtest/include
LIB_FLAGS:= -L/usr/src/gtest -lgtest -lgtest_main

.PHONY: clean
clean:
	rm delegate_test_11.out delegate_test_14.out delegate_test_17.out

delegate_test_11.out: test/delegate_test.cpp include/delegate/delegate.hpp
	g++ -std=c++11 $(INC_FLAGS) -o delegate_test_11.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	g++ -std=c++14 $(INC_FLAGS) -o delegate_test_14.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	g++ -std=c++17 $(INC_FLAGS) -o delegate_test_17.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread

run_test: delegate_test_11.out
	./delegate_test_11.out && ./delegate_test_14.out && ./delegate_test_17.out

.PHONY: format
format:
	clang-format-6.0 -i include/delegate/delegate.hpp
	clang-format-6.0 -i test/delegate_test.cpp
