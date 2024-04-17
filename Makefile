all: buildDir handleDependencies dynamicLibs staticLibs executables
buildDir:
	mkdir -p build
dynamicLibs: build/libmapper.so build/libmapperjni.so 
staticLibs: build/libmapper.a build/libkvclient.a 
executables: build/server build/rust-server build/mapperTest build/clientTest build/benchmark build/Main.jar 
handleDependencies: 
build/libmapper.so : src/mapper.c
	cc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/mapper.c -o src/mapper.c.o
	cc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -shared src/mapper.c.o -o build/libmapper.so
build/libmapperjni.so : src/mapper.c src/jni_wrapper.c
	gcc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -I/usr/lib/jvm/java-17-openjdk/include -I/usr/lib/jvm/java-17-openjdk/include/linux -c src/mapper.c -o src/mapper.c.o
	gcc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -I/usr/lib/jvm/java-17-openjdk/include -I/usr/lib/jvm/java-17-openjdk/include/linux -c src/jni_wrapper.c -o src/jni_wrapper.c.o
	gcc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -I/usr/lib/jvm/java-17-openjdk/include -I/usr/lib/jvm/java-17-openjdk/include/linux -shared src/mapper.c.o src/jni_wrapper.c.o -o build/libmapperjni.so
build/libmapper.a : src/mapper.c
	cc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/mapper.c -o src/mapper.c.o -static
	ar rcs build/libmapper.a src/mapper.c.o
build/libkvclient.a : src/client.cpp
	g++ -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
	ar rcs build/libkvclient.a src/client.cpp.o
build/server: src/server.cpp staticLibs
	g++ -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/server.cpp -o src/server.cpp.o -static
	g++ -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ src/server.cpp.o -o build/server -Lbuild -lmapper -static
build/rust-server: src/server.rs dynamicLibs
	rustc -C target-feature=+crt-static src/server.rs -o build/rust-server -L build/ -l static=mapper
build/mapperTest: src/mapper.c staticLibs
	cc -DTEST -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/mapper.c -o src/mapper.c.o -static
	cc -DTEST -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ src/mapper.c.o -o build/mapperTest -static
build/clientTest: src/client.cpp staticLibs
	g++ -DTEST -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
	g++ -DTEST -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ src/client.cpp.o -o build/clientTest -static
build/benchmark: src/client.cpp staticLibs
	g++ -DTEST -DBENCHMARK -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
	g++ -DTEST -DBENCHMARK -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ src/client.cpp.o -o build/benchmark -static
build/Main.jar: src/MapperWrapper.java src/Main.java dynamicLibs
	javac -d ./build/classes src/MapperWrapper.java src/Main.java
	cp -r META-INF/ build/
	cp build/libmapperjni.so build/classes/
	jar cfm build/Main.jar build/META-INF/MANIFEST.MF -C build/classes .
clean:
	rm -rf build/
	rm -f src/mapper.c.o
	rm -f src/mapper.c.o
	rm -f src/server.cpp.o
	rm -f src/client.cpp.o
	rm -f src/server.rs.o
	rm -f src/mapper.c.o
	rm -f src/client.cpp.o
	rm -f src/client.cpp.o
	rm -f src/mapper.c.o
	rm -f src/jni_wrapper.c.o
	rm -f src/MapperWrapper.java.o
	rm -f src/Main.java.o