mkdir -p build
cc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/mapper.c -o src/mapper.c.o
cc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -shared src/mapper.c.o -o build/libmapper.so
gcc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -I/usr/lib/jvm/java-17-openjdk/include -I/usr/lib/jvm/java-17-openjdk/include/linux -c src/mapper.c -o src/mapper.c.o
gcc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -I/usr/lib/jvm/java-17-openjdk/include -I/usr/lib/jvm/java-17-openjdk/include/linux -c src/jni_wrapper.c -o src/jni_wrapper.c.o
gcc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -I/usr/lib/jvm/java-17-openjdk/include -I/usr/lib/jvm/java-17-openjdk/include/linux -shared src/mapper.c.o src/jni_wrapper.c.o -o build/libmapperjni.so
cc -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/mapper.c -o src/mapper.c.o -static
ar rcs build/libmapper.a src/mapper.c.o
g++ -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
ar rcs build/libkvclient.a src/client.cpp.o
g++ -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/server.cpp -o src/server.cpp.o -static
g++ -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ src/server.cpp.o -o build/server -Lbuild -lmapper -static
rustc -C target-feature=+crt-static src/server.rs -o build/rust-server -L build/ -l static=mapper
cc -DTEST -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/mapper.c -o src/mapper.c.o -static
cc -DTEST -std=c99 -fPIC -O3 -Idependencies/logger -Iinclude/ src/mapper.c.o -o build/mapperTest -static
g++ -DTEST -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
g++ -DTEST -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ src/client.cpp.o -o build/clientTest -static
g++ -DTEST -DBENCHMARK -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
g++ -DTEST -DBENCHMARK -std=c++17 -fPIC -O3 -Idependencies/logger -Iinclude/ src/client.cpp.o -o build/benchmark -static
javac -d ./build/classes src/MapperWrapper.java src/Main.java
cp -r META-INF/ build/
cp build/libmapperjni.so build/classes/
jar cfm build/Main.jar build/META-INF/MANIFEST.MF -C build/classes .
