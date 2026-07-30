CXX := g++
CXXFLAGS := -g -O0 -Wall -Wextra -std=c++17

cpp_symbols: demo.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f cpp_symbols
