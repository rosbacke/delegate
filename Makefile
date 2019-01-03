all: run_test


PICKY_FLAGS:=-Wall -Wextra  -Wstrict-aliasing -pedantic -Werror -Wunreachable-code -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo  -Wstrict-overflow=5 -Wswitch-default -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option
PICKY_FLAGS_G:=-Wstrict-null-sentinel -Wnoexcept  -Wlogical-op

INC_FLAGS:= -Iinclude -I/usr/src/gtest/include
LIB_FLAGS:= -L/usr/src/gtest -lgtest -lgtest_main -pthread
CXX_FLAGS=-Wall -pedantic $(PICKY_FLAGS)

GCC_FLAGS:=$(PICKY_FLAGS_G)


.PHONY: clean
clean:
	rm delegate_test_11_g.out delegate_test_14_g.out delegate_test_17_g.out
	rm delegate_test_11_c.out delegate_test_14_c.out delegate_test_17_c.out

delegate_test_11.out: test/delegate_test.cpp include/delegate/delegate.hpp
	g++ -std=c++11 $(CXX_FLAGS) $(INC_FLAGS) $(GCC_FLAGS) -o delegate_test_11_g.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS)
	g++ -std=c++14 $(CXX_FLAGS) $(INC_FLAGS) $(GCC_FLAGS) -o delegate_test_14_g.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS)
	g++ -std=c++17 $(CXX_FLAGS) $(INC_FLAGS) $(GCC_FLAGS) -o delegate_test_17_g.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS)
	clang++ -std=c++11 $(CXX_FLAGS) $(INC_FLAGS) -o delegate_test_11_c.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS)
	clang++ -std=c++14 $(CXX_FLAGS) $(INC_FLAGS) -o delegate_test_14_c.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS)
	clang++ -std=c++17 $(CXX_FLAGS) $(INC_FLAGS) -o delegate_test_17_c.out -Iinclude test/delegate_test.cpp $(LIB_FLAGS)

run_test: delegate_test_11.out
	./delegate_test_11_g.out && ./delegate_test_14_g.out && ./delegate_test_17_g.out && 	./delegate_test_11_c.out && ./delegate_test_14_c.out && ./delegate_test_17_c.out


.PHONY: format
format:
	clang-format-6.0 -i include/delegate/delegate.hpp
	clang-format-6.0 -i test/delegate_test.cpp
