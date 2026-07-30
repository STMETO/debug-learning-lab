CXX := g++
all: parse_demo
person.pb.h person.pb.cc: person.proto
	protoc --cpp_out=. person.proto
parse_demo: main.cc person.pb.cc person.pb.h
	$(CXX) -g -Wall -Wextra -std=c++17 -o $@ main.cc person.pb.cc -lprotobuf
.PHONY: clean
clean:
	rm -f parse_demo person.pb.*
