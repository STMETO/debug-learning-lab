PROTOC := protoc
all: person.pb.h person.pb.cc
person.pb.h person.pb.cc: person.proto
	$(PROTOC) --cpp_out=. $<
.PHONY: clean
clean:
	rm -f person.pb.h person.pb.cc
