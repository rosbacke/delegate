all: run_test

INC_FLAGS:= -Iinclude -I/usr/src/gtest/include
LIB_FLAGS:= -L/usr/src/gtest -lgtest -lgtest_main

.PHONY: clean
clean:
	rm delegate_test_11_g.out delegate_test_14_g.out delegate_test_17_g.out
	rm delegate_test_11_c.out delegate_test_14_c.out delegate_test_17_c.out

delegate_test_11.out: test/delegate_test.cpp include/delegate/delegate.hpp
	g++ -std=c++11 -Wall $(INC_FLAGS) -o delegate_test_11_g.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	g++ -std=c++14 -Wall $(INC_FLAGS) -o delegate_test_14_g.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	g++ -std=c++17 -Wall $(INC_FLAGS) -o delegate_test_17_g.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	clang++ -std=c++11 -Wall $(INC_FLAGS) -o delegate_test_11_c.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	clang++ -std=c++14 -Wall $(INC_FLAGS) -o delegate_test_14_c.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread
	clang++ -std=c++17 -Wall $(INC_FLAGS) -o delegate_test_17_c.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS) -pthread

run_test: delegate_test_11.out
	./delegate_test_11_g.out && ./delegate_test_14_g.out && ./delegate_test_17_g.out && 	./delegate_test_11_c.out && ./delegate_test_14_c.out && ./delegate_test_17_c.out


.PHONY: format
format:
	clang-format-6.0 -i include/delegate/delegate.hpp
	clang-format-6.0 -i test/delegate_test.cpp
