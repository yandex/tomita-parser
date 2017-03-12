CC=g++
LD=g++
PC=protoc
CFLAGS=-c -g -Wall `pkg-config --cflags protobuf`
LDFLAGS=`pkg-config --libs protobuf`

all: parsefacts 

parsefacts: main.o facts.pb.o
	$(LD) main.o facts.pb.o $(LDFLAGS) -o parsefacts

main.o: main.cpp facts.pb.cc
	$(CC) $(CFLAGS) main.cpp

facts.pb.o: facts.pb.cc
	$(CC) $(CFLAGS) facts.pb.cc

facts.pb.cc: ../facts.proto
	$(PC) --proto_path=.. ../facts.proto --cpp_out=./

clean:
	rm -rf main.o facts.pb.* parsefacts
