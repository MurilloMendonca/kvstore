# C flags
FLAGS_DB = -Wall -g -O0 -pedantic -Wextra -Werror -fsanitize=address -fsanitize=undefined -mshstk -DDEBUG
FLAGS_RL = -O3
FLAGS = $(FLAGS_RL)
ifeq ($(DEBUG), 1)
	FLAGS = $(FLAGS_DB)
endif
CFLAGS = $(FLAGS) -std=c99
CCFLAGS = $(FLAGS) -std=c++17

all: libmapper.so libmapper.a server libkvclient.so libkvclient.a clientTest mapperTest

zig:
	zig build-lib -dynamic mapper.zig  -OReleaseFast -lc
rust:
	rustc server.rs -L. -l dylib=mapper -o server -C link-arg=-Wl,-rpath,.
clean:
	rm -rf *.so *.a *.o server mapperTest clientTest benchmark 

libmapper.so: mapper.o
	cc -shared -o libmapper.so mapper.o

libmapper.a: mapper.o
	ar rcs libmapper.a mapper.o

mapper.o: mapper.c
	cc -c mapper.c -fPIC -o mapper.o $(CFLAGS)

server: server.cpp libmapper.a
	g++  server.cpp -o server -L. -l:libmapper.so $(CCFLAGS) -Wl,-rpath,.

kvclient.o: client.cpp
	g++ -c client.cpp -fPIC -o kvclient.o $(CCFLAGS)

libkvclient.so: kvclient.o
	g++ -shared -o libkvclient.so kvclient.o

libkvclient.a: kvclient.o
	ar rcs libkvclient.a kvclient.o

mapperTest: mapper.c 
	cc  -DTEST mapper.c -o mapperTest $(CFLAGS)

clientTest: client.cpp
	g++  -DTEST client.cpp -o clientTest $(CCFLAGS)

benchmark: client.cpp
	g++ -DBENCHMARK -DTEST client.cpp -o benchmark $(CCFLAGS)

test: mapperTest clientTest
	./mapperTest
	./clientTest

