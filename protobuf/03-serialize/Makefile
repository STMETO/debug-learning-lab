CXX := g++
CXXFLAGS := -g -Wall -Wextra -std=c++17
all: serialize_demo
person.pb.h person.pb.cc: person.proto
	protoc --cpp_out=. person.proto
serialize_demo: main.cc person.pb.cc person.pb.h
	$(CXX) $(CXXFLAGS) -o $@ main.cc person.pb.cc -lprotobuf
.PHONY: clean
clean:
	rm -f serialize_demo person.pb.* person.bin
